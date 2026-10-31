// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "projection.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace rchase2 {
struct NativeVertex {float x{},y{},z{},u{},v{};};
struct NativePolygon {
    unsigned count{},attr{},window{},sort{},luma{};int lod{};
    std::array<unsigned,4> header{};
    std::array<float,2> focus{},center{};
    std::array<int,4> viewport{};
    std::array<float,12> matrix{};
    std::array<NativeVertex,4> vertices;
    uint32_t object{};bool culled{};
};
struct Blob {std::string name;std::vector<uint8_t> bytes;};
struct NativeFrame {
    uint64_t sequence{};double time{};
    int xoffset{},yoffset{},width{},height{};
    bool complete{true};
    NativeLens lens;
    std::vector<uint32_t> background,foreground,native;
    unsigned rejected{};
    std::vector<NativePolygon> polygons;
    std::vector<Blob> blobs;
};
// This collector owns every byte. The producer keeps no mutable reference to
// the published frame, so an eye pass can safely retain its previous snapshot.
class SceneCapture {
    NativeFrame frame_;
    size_t total_bytes_{};
    size_t active_blobs_{};
public:
    static constexpr unsigned MaxPolygons=32768,MaxBlobBytes=4*1024*1024;
    static constexpr unsigned MaxFrameBytes=16*1024*1024,MaxBlobs=16;
    void begin(uint64_t sequence,double time) {
        if(!std::isfinite(time)||time<0)throw std::invalid_argument("Invalid native time");
        begin(sequence,time,NativeFrame{});
    }
    void begin(uint64_t sequence,double time,NativeFrame reusable){
        if(!std::isfinite(time)||time<0)throw std::invalid_argument("Invalid native time");
        frame_=std::move(reusable);frame_.sequence=sequence;frame_.time=time;
        frame_.xoffset=frame_.yoffset=frame_.width=frame_.height=0;
        frame_.complete=true;frame_.lens={};frame_.rejected=0;
        frame_.polygons.clear();frame_.background.clear();frame_.foreground.clear();frame_.native.clear();
        total_bytes_=active_blobs_=0;
    }
    void display(int xoffset,int yoffset,int width,int height) {
        if(width<1||height<1||width>2048||height>2048)throw std::invalid_argument("Invalid native display");
        frame_.xoffset=xoffset;frame_.yoffset=yoffset;frame_.width=width;frame_.height=height;
    }
    bool append(const NativePolygon& polygon) {
        bool valid=polygon.count==3||polygon.count==4;
        for(unsigned i=0;valid&&i<polygon.count;i++) {
            const auto& v=polygon.vertices[i];
            valid=std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&std::isfinite(v.u)&&std::isfinite(v.v);
        }
        for(float value:polygon.focus)valid=valid&&std::isfinite(value);
        for(float value:polygon.center)valid=valid&&std::isfinite(value);
        for(float value:polygon.matrix)valid=valid&&std::isfinite(value);
        if(!valid||frame_.polygons.size()>=MaxPolygons){++frame_.rejected;return false;}
        frame_.polygons.push_back(polygon);return true;
    }
    void bytes(const char* name,const void* data,size_t length) {
        if(!name||!data||!length||length>MaxBlobBytes||active_blobs_>=MaxBlobs||length>MaxFrameBytes-total_bytes_)
            throw std::invalid_argument("Invalid native snapshot size");
        std::string key=name;
        if(key.empty()||key.size()>64||key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos)
            throw std::invalid_argument("Invalid native snapshot name");
        for(size_t i=0;i<active_blobs_;i++)if(frame_.blobs[i].name==key)throw std::invalid_argument("Duplicate native snapshot name");
        const auto* first=static_cast<const uint8_t*>(data);
        if(active_blobs_==frame_.blobs.size())frame_.blobs.emplace_back();
        auto& blob=frame_.blobs[active_blobs_++];blob.name=std::move(key);blob.bytes.assign(first,first+length);total_bytes_+=length;
    }
    NativeFrame take() {frame_.blobs.resize(active_blobs_);auto frame=std::move(frame_);frame_={};total_bytes_=active_blobs_=0;return frame;}
    std::shared_ptr<const NativeFrame> finish() {return std::make_shared<const NativeFrame>(take());}
};
}
