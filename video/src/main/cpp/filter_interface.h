//
// Created by Administrator on 2024/8/17.
//

#ifndef MEDIAPACKAGE_FILTER_INTERFACE_H
#define MEDIAPACKAGE_FILTER_INTERFACE_H

#include "base_interface.h"

extern "C" {
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/avfilter.h"
};

/**
 * 补充滤镜接口
 *  * 使用滤镜主要为以下步骤
 *  初始化FFmpeg库：与之前的步骤相同。
    创建滤镜图：通过avfilter_graph_parse_ptr来解析滤镜图字符串，自动创建并连接滤镜。
    配置滤镜图：使用avfilter_graph_config配置滤镜图。//连接滤镜：将滤镜连接在一起，形成一个滤镜图。
    处理视频帧：通过滤镜图处理视频帧。解析滤镜的时候需要到avframe中进行传递来具体解析
   // 发送帧到滤镜图
    if (av_buffersrc_add_frame(buffersrc_ctx, frame) < 0) {
        fprintf(stderr, "Error while feeding the filtergraph\n");
        break;
    }
    // 从滤镜图获取帧
    while (1) {
        ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
 */
class FilterInterface : public BaseInterface {

public:
    AVFilterGraph *filterGraph;
    AVFilterContext *bufferSrcCtx;
    AVFilterContext *bufferSinkCtx;


    FilterInterface();

    ~FilterInterface();

    /**
    链接滤镜有两种方案：
    第一种：
    ret = avfilter_link(buffersrc_ctx, 0, blur_filter_ctx, 0);
    if (ret >= 0) {
        ret = avfilter_link(blur_filter_ctx, 0, buffersink_ctx, 0);
    }
    第二种
    解析滤镜图字符串
    ret = avfilter_graph_parse_ptr(filter_graph, filter_desc, &buffersrc_ctx, &buffersink_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error while parsing filter graph\n");
     */
    int initFilter(const char *filters_descr, int videoIndex, AVRational videoTimebase,
                   AVCodecContext *dec_ctx);

};


#endif //MEDIAPACKAGE_FILTER_INTERFACE_H
