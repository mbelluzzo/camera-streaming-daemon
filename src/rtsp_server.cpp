/*
 * This file is part of the Camera Streaming Daemon
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sstream>
#include <string.h>

#include "rtsp_server.h"
#include "log.h"

#define STREAM_TYPE_RTSP_MEDIA_FACTORY stream_rtsp_media_factory_get_type()
G_DECLARE_FINAL_TYPE(StreamRTSPMediaFactory, stream_rtsp_media_factory, STREAM, RTSP_MEDIA_FACTORY,
                     GstRTSPMediaFactory)
struct _StreamRTSPMediaFactory {
    GstRTSPMediaFactory parent;
    RTSPServer *rtsp_server;
};

G_DEFINE_TYPE(StreamRTSPMediaFactory, stream_rtsp_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY)

GstElement *stream_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
    return ((StreamRTSPMediaFactory *)factory)->rtsp_server->create_element_from_url(url);
}

static void stream_rtsp_media_factory_class_init(StreamRTSPMediaFactoryClass *klass)
{
    GstRTSPMediaFactoryClass *factory = (GstRTSPMediaFactoryClass *)(klass);
    factory->create_element = stream_create_element;
}

static void stream_rtsp_media_factory_init(StreamRTSPMediaFactory *self)
{
}

static GstRTSPMediaFactory *create_media_factory(RTSPServer *server)
{
    StreamRTSPMediaFactory *factory
        = (StreamRTSPMediaFactory *)g_object_new(STREAM_TYPE_RTSP_MEDIA_FACTORY, nullptr);
    factory->rtsp_server = server;
    return (GstRTSPMediaFactory *)factory;
}

RTSPServer::RTSPServer(std::vector<std::unique_ptr<Stream>> &_streams, int _port)
    : streams(_streams)
    , is_running(false)
    , port(_port)
    , server(nullptr)
{
    gst_init(nullptr, nullptr);
}

RTSPServer::~RTSPServer()
{
    stop();
}

void RTSPServer::start()
{
    gint server_handle;

    if (is_running)
        return;
    is_running = true;

    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", nullptr);

    if (streams.size() > 0) {
        mounts = gst_rtsp_server_get_mount_points(server);
        factory = create_media_factory(this);
        for (auto &s : streams)
            gst_rtsp_mount_points_add_factory(mounts, s->get_path().c_str(), factory);
        g_object_unref(mounts);
    }

    server_handle = gst_rtsp_server_attach(server, nullptr);
    if (!server_handle) {
        log_error("Unable to start rtsp server");
        stop();
    }
}

void RTSPServer::stop()
{
    if (!is_running)
        return;
    is_running = false;

    g_object_unref(server);

    server = nullptr;
}

Stream *RTSPServer::find_stream_by_path(const char *path)
{
    for (auto &s : streams)
        if (s->get_path() == path)
            return s.get();

    return nullptr;
}

void RTSPServer::append_to_map(std::map<std::string, std::string> &map, const std::string &param)
{
    size_t j = param.find('=');
    if (j == std::string::npos)
        return;

    map[param.substr(0, j)] = param.substr(j + 1);
}

std::map<std::string, std::string> RTSPServer::parse_uri_query(const char *query)
{
    std::map<std::string, std::string> map;

    if (!query || !query[0])
        return map;

    std::string query_str = query;
    size_t i = 0, j;

    j = query_str.find('&');
    while (j != std::string::npos) {
        append_to_map(map, query_str.substr(i, j - i));
        i = j + 1;
        j = query_str.find('&', j + 1);
    }
    append_to_map(map, query_str.substr(i, j - i));

    return map;
}

GstElement *RTSPServer::create_element_from_url(const GstRTSPUrl *url)
{
    GstElement *pipeline;
    Stream *stream;
    std::map<std::string, std::string> params;

    log_debug("RTSP request: %s (query: %s)", url->abspath, url->query);
    stream = find_stream_by_path(url->abspath);
    if (!stream)
        goto error;

    params = parse_uri_query(url->query);
    pipeline = stream->get_gstreamer_pipeline(params);
    if (!pipeline)
        goto error;

    return pipeline;
error:
    log_error("No gstreamer pipeline available for request (device_path: %s - query: %s)",
              url->abspath, url->query);
    return nullptr;
}
