#include "OCRProcess.h"
#include "tesseract/baseapi.h"
#include "leptonica/allheaders.h"
#include "leptonica/pix_internal.h"
#include <QDebug>

Pix* preprocessImage(const QString& filepath) {
    Pix* original = pixRead(filepath.toStdString().c_str());
    if (!original) return nullptr;

    // 获取原始图像信息
    l_int32 width = pixGetWidth(original);
    l_int32 height = pixGetHeight(original);
    l_int32 dpi = 300; // 目标 DPI

    qDebug() << QString("原始图像: %1x%2, DPI: %3")
                    .arg(width).arg(height).arg(pixGetXRes(original));

    Pix* processed = nullptr;

    // 1. 转换为灰度图（如果原图是彩色）
    Pix* gray = nullptr;
    if (pixGetDepth(original) > 8) {
        gray = pixConvertRGBToGray(original, 0.0, 0.0, 0.0);
        qDebug() << "已转换为灰度图";
    } else {
        gray = pixClone(original);
    }

    // 2. 检查并调整分辨率
    l_int32 currentDpi = pixGetXRes(gray);
    if (currentDpi < 1) {
        // 没有 DPI 信息，假设是 96 DPI（屏幕分辨率）
        currentDpi = 96;
    }

    // 3. 计算缩放比例（目标 300 DPI）
    float scale = 300.0f / currentDpi;
    if (scale < 0.5f || scale > 2.0f) {
        // 限制缩放范围，避免过度失真
        scale = qBound(0.5f, scale, 2.0f);
    }

    // 4. 应用缩放
    if (fabs(scale - 1.0f) > 0.05f) { // 变化超过5%才缩放
        qDebug() << QString("缩放图像: 比例 %1").arg(scale);
            processed = pixScaleGrayLI(gray, scale, scale);
    } else {
        processed = pixClone(gray);
    }

    // 5. 设置 DPI
    if (processed) {
        pixSetXRes(processed, dpi);
        pixSetYRes(processed, dpi);
    }

    // 清理临时图像
    pixDestroy(&gray);
    pixDestroy(&original);

    return processed;
}

OCRProcess *OCRProcess::Instance()
{
    static OCRProcess* m_instance = new OCRProcess();
    return m_instance;
}

OCRProcess::~OCRProcess()
{
    if(m_api)
    {
        if (m_isLoad)
            m_api->End();
        delete m_api;
        m_api = nullptr;
    }
}

OCRProcess::OCRProcess()
{
    m_api = nullptr;
    m_isLoad = false;
    m_stoploop = true;
    Init();
}

bool OCRProcess::Init()
{
    m_api = new tesseract::TessBaseAPI();
    const std::string tessdata_path = "./tessdata";

    int result = m_api->Init(tessdata_path.c_str(), "chi_sim");
    if (result != 0)
    {
        qDebug()<<("Could not initialize tesseract.\n");
        delete m_api;
        m_api = nullptr;
        m_isLoad = false;
    }
    else
    {
        m_isLoad = true;
    }

    if(m_isLoad)
    {
        m_stoploop = false;
        std::thread T(&OCRProcess::processLoop,this);
        T.detach();
    }

    return m_isLoad;
}

bool OCRProcess::isLoaded() const
{
    return m_isLoad;
}

void OCRProcess::addOCRTask(std::shared_ptr<OCRTask> task)
{
    if(!task)
        return;
    m_tasks.enqueue(task);
    LockGuard guard(m_looplock);
    m_loopcv.NotifyOne();
}

void OCRProcess::processLoop()
{
    while(!m_stoploop)
    {
        LockGuard guard(m_looplock);
        m_loopcv.WaitFor(guard,std::chrono::milliseconds(100));

        if(m_stoploop)
            break;

        if(m_tasks.empty())
            continue;

        std::shared_ptr<OCRTask> task;
        if(m_tasks.dequeue(task) && task)
        {
            processTask(task);
        }
    }
}

void OCRProcess::processTask(std::shared_ptr<OCRTask> task)
{
    if(task->filepath.isEmpty())
    {
        task->state = OCRTaskResult::Error;
        if(task->callback)
            task->callback(task);
        return;
    }

    Pix *image =  pixRead(task->filepath.toStdString().c_str());
    // Pix *image = preprocessImage(task->filepath);

    if(!image)
    {
        task->state = OCRTaskResult::Error;
        if(task->callback)
            task->callback(task);
        return;
    }

    m_api->SetImage(image);

    Boxa* boxes = m_api->GetComponentImages(tesseract::RIL_TEXTLINE, true, NULL, NULL);
    if(boxes)
    {
        qDebug()<<QString("Found %1 textline image components.\n").arg(boxes->n);
        for (int i = 0; i < boxes->n; i++) {
            BOX* box = boxaGetBox(boxes, i, L_CLONE);
            m_api->SetRectangle(box->x, box->y, box->w, box->h);

            char* ocrResult = m_api->GetUTF8Text();
            int conf = m_api->MeanTextConf();

            QString QSocrResult = QString::fromUtf8(ocrResult);

            qDebug()<<QString("Box[%1]: x=%2, y=%3, w=%4, h=%5, confidence: %6, text: %7")
                            .arg(i).arg(box->x).arg(box->y).arg(box->w).arg(box->h).arg(conf).arg(QSocrResult);

            task->boxs.emplace_back(OCRBox{box->x,box->y,box->w,box->h,conf,QSocrResult});

            boxDestroy(&box);
            if (ocrResult) {
                // delete[] ocrResult;
            }
        }
        boxaDestroy(&boxes);
    }
    else
    {
        qDebug()<<QString("Found %1 textline image components.\n").arg(0);
    }

    pixDestroy(&image);

    m_api->Clear();

    task->state = OCRTaskResult::Success;
    if(task->callback)
        task->callback(task);
}

