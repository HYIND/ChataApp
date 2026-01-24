#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include "ConnectManager.h"
#include <QQmlContext>
#include "ModelManager.h"
#include "RequestManager.h"
#include "3DModelViewerModel.h"
#include <QQuickWindow>
#include "LlamaModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    LlamaModel::Instance()->load("./chatmodel.gguf");
    if(LlamaModel::Instance()->isLoaded())
    {
        std::thread T(&LlamaModel::runasyncprocess,LlamaModel::Instance());
        T.detach();
    }

    qmlRegisterType<OpenGLModelRenderItem>("OpenGLRendering", 1, 0, "OpenGLRenderItem");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    // const QUrl url(QStringLiteral("qrc:/qml/LlamaChatWindow.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);


    // 将模型实例暴露给QML
    engine.rootContext()->setContextProperty("chatitemlistmodel", CHATITEMMODEL);
    engine.rootContext()->setContextProperty("sessionmodel", SESSIONMODEL);
    engine.rootContext()->setContextProperty("userinfomodel", USERINFOMODEL);
    engine.rootContext()->setContextProperty("loginmodel", LOGINMODEL);
    engine.rootContext()->setContextProperty("aiassistantmodel", AIASSISTANTMODEL);
    engine.rootContext()->setContextProperty("aisummarymodel", AISUMMARYMODEL);
    engine.rootContext()->setContextProperty("ocrmodel", OCRMODEL);
    engine.load(url);

    // 创建局部事件循环和超时控制器等待子线程完成初始化
    QEventLoop localLoop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(10000);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &localLoop,
                     &QEventLoop::quit);

    // 创建工作线程处理网络连接
    QThread* connectWorkerThread = new QThread;
    // 连接线程启动信号
    QObject::connect(connectWorkerThread, &QThread::started, [&localLoop,connectWorkerThread]() {
    // 在 workerThread 中创建对象树（确保所有对象属于该线程）
            CONNECTMANAGER;
            // 此时 worker 及其所有子对象均属于 workerThread
            // qDebug() << "Worker thread:" << CONNECTMANAGER->thread();
            QMetaObject::invokeMethod(&localLoop, "quit");
    });
    connectWorkerThread->start();

    // 进入局部事件循环（非阻塞式等待）
    localLoop.exec();

    CONNECTMANAGER->Login("192.168.58.130",8888);

    int trycount = 0;
    while(trycount < 10)
    {
        auto status = CONNECTMANAGER->connectStatus();
        if (status == ConnectStatus::connected || status == ConnectStatus::disconnect)
            break;
        trycount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (CONNECTMANAGER->connectStatus() == ConnectStatus::connected)
    {
        LOGINMODEL->LoginSuccess();

        // 创建定时器
        QTimer *timer = new QTimer();

        // 连接定时器的 timeout 信号到槽函数
        QObject::connect(timer, &QTimer::timeout, [&]() {
            REQUESTMANAGER->RequestOnlineUserData();
        });
        timer->start(3000);
    }
    else {
        LOGINMODEL->LoginFail();
    }

    return app.exec();
}
