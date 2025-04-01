//
// Created by Administrator on 2024/10/27.
//

#include "opencv_jni.h"
#include <jni.h>
#include "opencv2/opencv.hpp"
#include <iostream>
#include "sys/stat.h"

using namespace cv;

class CascadeDetectorAdapter : public cv::DetectionBasedTracker::IDetector {
public:
    //构造函数
    CascadeDetectorAdapter(cv::Ptr<cv::CascadeClassifier> detector) :IDetector(), Detector(detector) {

    }

    void detect(const cv::Mat &image, std::vector<cv::Rect> &objects) override {
        Detector->detectMultiScale(image, objects, scaleFactor, minNeighbours, 0, minObjSize,
                                   maxObjSize);
    }

private:
    CascadeDetectorAdapter();
    cv::Ptr<cv::CascadeClassifier> Detector;
};

cv::DetectionBasedTracker *tracker;
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_opencv_OpencvJni_init(JNIEnv *env, jobject thiz, jstring path) {
    const char *pPath = env->GetStringUTFChars(path, 0);
    //创建检测器
    Ptr<CascadeClassifier> classifier = makePtr<CascadeClassifier>(pPath);
    Ptr<CascadeDetectorAdapter> mainDetector = makePtr<CascadeDetectorAdapter>(classifier);
    //创建跟踪器
    Ptr<CascadeClassifier> classifier1 = makePtr<CascadeClassifier>(pPath);
    Ptr<CascadeDetectorAdapter> trackerDedetor = makePtr<CascadeDetectorAdapter>(classifier1);

    DetectionBasedTracker::Parameters param;
    tracker = new DetectionBasedTracker(mainDetector, trackerDedetor, param);
    tracker->run();
    LOGI("init success");
    env->ReleaseStringUTFChars(path, pPath);
}
int index = 0;
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_opencv_OpencvJni_postData(JNIEnv *env, jobject thiz, jbyteArray byte_array, jint w,
                                           jint h, jint camera_id) {
    if (index > 10) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(byte_array, NULL);
    //Mat创建的两种写法
//    Mat *mat = new Mat(h + h / 2, w, CV_8UC1, data);
    Mat src(h + h / 2, w, CV_8UC1, data);
    //yuv转成rgb
    cvtColor(src, src, COLOR_YUV2RGBA_NV21);

    //旋转摄像头
    if (camera_id == 0) {//前置摄像头`
        rotate(src, src, RotateFlags::ROTATE_90_COUNTERCLOCKWISE);
        flip(src, src, 1);
    } else {
        rotate(src, src, RotateFlags::ROTATE_90_COUNTERCLOCKWISE);
    }
    mkdir("/sdcard/Pictures/opencv", 0777);
    char fileName[100];

    //转灰色
    Mat gray;
    cvtColor(src,gray,COLOR_RGB2GRAY);
    //转对比度(增强对比度，直方图增强)
    equalizeHist(gray,gray);
    //进行追踪
    tracker->process(gray);
    std::vector<Rect> faces;
    tracker->getObjects(faces);
    for(Rect rect:faces){
        sprintf(fileName, "/sdcard/Pictures/opencv/%d.jpg", index++);
        Mat m;
        m=gray(rect).clone();
        imwrite(fileName, m);
    }

}