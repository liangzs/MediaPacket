//
// Created by Administrator on 2024/8/17.
//

#include "filter_interface.h"

FilterInterface::FilterInterface() {

}

FilterInterface::~FilterInterface() {

}

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
int FilterInterface::initFilter(const char *filters_descr, int videoIndex, AVRational videoTimebase,
                                AVCodecContext *dec_ctx) {
    char args[512];
    int ret;
    //固定流程
    AVFilter *bufferSrc = const_cast<AVFilter *>(avfilter_get_by_name("buffer"));
    AVFilter *bufferSink = const_cast<AVFilter *>(avfilter_get_by_name("buffersink"));

    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();
    filterGraph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filterGraph) {
        ret = AVERROR(ENOMEM);
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        LOGE("avfilter_graph_alloc faild !");
        return -1;
    }
// 创建buffer源滤镜
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             dec_ctx->time_base.num, dec_ctx->time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);
//    avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("buffer"),
//                                 "in", args, NULL, filter_graph);

    //另外一个方案，直接在create_filter中实现，不用在sink中链接自定义滤镜
//    ret = avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("buffer"),
//                                       "in", args, NULL, filter_graph);
//    if (ret < 0) {
//        fprintf(stderr, "Could not create buffer source\n");
//        exit(1);
//    }
//    // 创建buffer sink滤镜
//    ret = avfilter_graph_create_filter(&buffersink_ctx, avfilter_get_by_name("buffersink"),
//    if (ret < 0) {
//        fprintf(stderr, "Could not create buffer sink\n");
//        exit(1);
//    }
//    // 添加自定义滤镜
//    AVFilterContext *blur_filter_ctx;
//    ret = avfilter_graph_create_filter(&blur_filter_ctx, avfilter_get_by_name("gblur"),
//    if (ret < 0) {
//        fprintf(stderr, "Could not create blur filter\n");
//        exit(1);
//    }

//create Filter avfilter_graph_create_filter: 用于在滤镜图中创建和初始化滤镜。先in后out，然后目的滤镜
//avfilter_link: 用于将两个滤镜连接在一起，这样视频帧可以通过整个滤镜链。
    ret = avfilter_graph_create_filter(&bufferSrcCtx, bufferSrc, "in", args, NULL, filterGraph);
    if (ret < 0) {
        LOGE("avfilter_graph_create_filter fail");
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return -1;
    }
    ret = avfilter_graph_create_filter(&bufferSinkCtx, bufferSink, "out", NULL, NULL, filterGraph);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        LOGE("");
        return -1;
    }
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    ret = av_opt_set_int_list(bufferSinkCtx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        LOGE("av_opt_set_int_list fail");
        return -1;
    }

    /*
    * Set the endpoints for the filter graph. The filter_graph will
    * be linked to the graph described by filters_descr.
    */
    outputs->name = av_strdup("in");
    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->filter_ctx = bufferSrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name = av_strdup("out");
    inputs->filter_ctx = bufferSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    //重点来了，自定义滤镜
    ret = avfilter_graph_parse_ptr(filterGraph, filters_descr, &inputs, &outputs, NULL);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        LOGE("avfilter_graph_parse_ptr fail");
        return -1;
    }
    //另外一种实现 // 连接滤镜
    //    ret = avfilter_link(buffersrc_ctx, 0, blur_filter_ctx, 0);
    //    if (ret >= 0) {
    //        ret = avfilter_link(blur_filter_ctx, 0, buffersink_ctx, 0);
    //    }
    //
    // 配置滤镜图
    ret = avfilter_graph_config(filterGraph, NULL);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        LOGE("avfilter_graph_config fail");
        return -1;
    }
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return 0;
}
