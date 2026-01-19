#include "AsyncMD5.h"
#include "MD5Helper.h"
#include "ThreadPool.h"

class AsyncMD5Handle : public std::enable_shared_from_this<AsyncMD5Handle>
{
    enum class execstatus
    {
        idle = 0,
        running = 1
    };

public:
    AsyncMD5Handle();
    ~AsyncMD5Handle();

    void PushData(Buffer &data);
    void PushDataSync(Buffer &data);
    std::string Final();

    std::shared_ptr<MD5Context> MD5Ctx();
    uint64_t Count();
    MD5SnapShot SnapShot();
    void LoadSnapShot(MD5SnapShot &snapshot);

    void ProcessData();
    void StopCalculate();

private:
    static CriticalSectionLock _execpoolmutex;
    static ThreadPool _execpool;

    void Need();

private:
    uint64_t _datacount;
    SafeQueue<std::shared_ptr<Buffer>> _pushqueue;
    std::shared_ptr<MD5Context> _ctx;
    std::string _md5string;

    CriticalSectionLock _processlock;
    execstatus _status;

    CriticalSectionLock _updateCtxlock;

    std::atomic<bool> can_final{true};
    bool _stop;
};

CriticalSectionLock AsyncMD5Handle::_execpoolmutex = CriticalSectionLock();
ThreadPool AsyncMD5Handle::_execpool = ThreadPool(2);

MD5SnapShot::MD5SnapShot()
{
    status[0] = 0x67452301;
    status[1] = 0xefcdab89;
    status[2] = 0x98badcfe;
    status[3] = 0x10325476;
    count = 0;
    isfinish = false;
}

MD5SnapShot::MD5SnapShot(MD5SnapShot &other)
{
    status[0] = other.status[0];
    status[1] = other.status[1];
    status[2] = other.status[2];
    status[3] = other.status[3];
    cacheBuffer = other.cacheBuffer;
    count = other.count;
    isfinish = other.isfinish;
}

MD5SnapShot::MD5SnapShot(const MD5SnapShot &other)
{
    status[0] = other.status[0];
    status[1] = other.status[1];
    status[2] = other.status[2];
    status[3] = other.status[3];
    cacheBuffer = other.cacheBuffer;
    count = other.count;
    isfinish = other.isfinish;
}

MD5SnapShot &MD5SnapShot::operator=(MD5SnapShot &other)
{
    if (this == &other)
        return *this;

    status[0] = other.status[0];
    status[1] = other.status[1];
    status[2] = other.status[2];
    status[3] = other.status[3];
    cacheBuffer = other.cacheBuffer;
    count = other.count;
    isfinish = other.isfinish;

    return *this;
}

AsyncMD5Handle::AsyncMD5Handle()
{
    _stop = false;
    _status = execstatus::idle;
    _datacount = 0;
    _ctx = MD5Helper::MD5Ctx_Init();
    Need();
}

AsyncMD5Handle::~AsyncMD5Handle()
{
    StopCalculate();
    if (_pushqueue.empty())
        _pushqueue.clear();
}

void AsyncMD5Handle::PushData(Buffer &data)
{
    if (data.Length() <= 0)
        return;

    auto buf = std::make_shared<Buffer>();
    buf->CopyFromBuf(data);

    {
        auto guard = _pushqueue.MakeLockGuard();
        _pushqueue.enqueue(buf);
        _datacount += buf->Length();
    }

    ProcessData();
}

void AsyncMD5Handle::PushDataSync(Buffer &data)
{
    if (data.Length() <= 0)
        return;

    auto buf = std::make_shared<Buffer>();
    buf->CopyFromBuf(data);

    {
        auto guard = _pushqueue.MakeLockGuard();
        _pushqueue.enqueue(buf);
        _datacount += buf->Length();
    }

    ProcessData();
    can_final.wait(false, std::memory_order_acquire);
}

std::string AsyncMD5Handle::Final()
{
    if (_md5string.empty())
    {
        can_final.wait(false, std::memory_order_acquire);
        _md5string = MD5Helper::MD5Ctx_Final(_ctx);
    }
    return _md5string;
}

std::shared_ptr<MD5Context> AsyncMD5Handle::MD5Ctx()
{
    return _ctx;
}

uint64_t AsyncMD5Handle::Count()
{
    return _datacount;
}


MD5SnapShot AsyncMD5Handle::SnapShot()
{
    MD5SnapShot shot;
    {
        LockGuard guard(_updateCtxlock);
        shot.status[0] = _ctx->status[0];
        shot.status[1] = _ctx->status[1];
        shot.status[2] = _ctx->status[2];
        shot.status[3] = _ctx->status[3];
        shot.count = _ctx->count;
        shot.cacheBuffer = _ctx->cacheBuffer;
        shot.isfinish = !_md5string.empty();
        if (shot.isfinish)
            shot.md5string = _md5string;
    }
    return shot;
}

void AsyncMD5Handle::LoadSnapShot(MD5SnapShot &snapshot)
{

    _ctx->status[0] = snapshot.status[0];
    _ctx->status[1] = snapshot.status[1];
    _ctx->status[2] = snapshot.status[2];
    _ctx->status[3] = snapshot.status[3];

    _ctx->cacheBuffer = snapshot.cacheBuffer;
    _ctx->count = snapshot.count;

    if (snapshot.isfinish)
        _md5string = snapshot.md5string;

    _datacount = snapshot.count + snapshot.cacheBuffer.Length();
    _pushqueue.clear();
}

void AsyncMD5Handle::StopCalculate()
{
    LockGuard guard(_processlock);
    _stop = true;
}

void AsyncMD5Handle::Need()
{
    LockGuard guard(_execpoolmutex);
    if (!_execpool.running())
        _execpool.start();
}

void AsyncMD5Handle::ProcessData()
{
    if (_stop)
        return;

    LockGuard guard(_processlock);
    if (_stop)
        return;

    if (_status == execstatus::idle) // 空闲
    {
        // 无任务，异步update完成
        if (_pushqueue.empty())
        {
            can_final.store(true, std::memory_order_release);
            can_final.notify_one();
        }
        else
        {
            // 有待处理的buf，异步处理
            std::shared_ptr<Buffer> buf;
            if (_pushqueue.dequeue(buf) && buf)
            {
                auto donecallback = [this]() -> void
                {
                    // 任务完成，设置空闲
                    _status = execstatus::idle;
                    ProcessData();
                };
                auto task = [&updatemutex = _updateCtxlock, md5ctx = _ctx, handle = shared_from_this(), block = buf, callback = donecallback]() -> void
                {
                    {
                        LockGuard guard(updatemutex);
                        MD5Helper::MD5Ctx_Update(md5ctx, reinterpret_cast<const unsigned char *>(block->Byte()), block->Length());
                    }
                    callback();
                };
                can_final.store(false, std::memory_order_release);
                _status = execstatus::running;
                _execpool.submit(task);
            }
        }
    }
}

AsyncMD5::AsyncMD5()
{
    _handle = std::make_shared<AsyncMD5Handle>();
}

AsyncMD5::AsyncMD5(MD5SnapShot &snapshot)
{
    _handle = std::make_shared<AsyncMD5Handle>();
    LoadSnapShot(snapshot);
}

AsyncMD5::~AsyncMD5()
{
    _handle->StopCalculate();
}

void AsyncMD5::Update(Buffer &data)
{
    _handle->PushData(data);
}

void AsyncMD5::UpdateSync(Buffer &data)
{
    _handle->PushDataSync(data);
}

std::string AsyncMD5::Final()
{
    if (_md5string.empty())
        _md5string = _handle->Final();
    return _md5string;
}

uint64_t AsyncMD5::Count()
{
    return _handle->Count();
}

MD5SnapShot AsyncMD5::SnapShot()
{
    return _handle->SnapShot();
}

void AsyncMD5::LoadSnapShot(MD5SnapShot &snapshot)
{
    _handle->LoadSnapShot(snapshot);
    if (snapshot.isfinish)
        _md5string = snapshot.md5string;
}
