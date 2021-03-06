//
//  data_converter.cpp
//  Filter
//
//  Created by 陈裕昕 on 1/1/15.
//  Copyright (c) 2015 Fudan. All rights reserved.
//

#include "data_converter.h"

namespace YuxinMap {
    Data_Converter::Data_Converter(int level, const pugi::xml_node &bounds, link_t &link, const ac_t &ac,
                                   const pugi::xml_node &conf, BG_Generator &bg_generator):
        ac(ac), conf(conf), link(link), level(level), bg_generator(bg_generator)
    {
        min_lat = bounds.attribute("minlat").as_double();
        max_lat = bounds.attribute("maxlat").as_double();
        min_lon = bounds.attribute("minlon").as_double();
        max_lon = bounds.attribute("maxlon").as_double();
        
        Coord tl(max_lat, min_lon, level), br(min_lat, max_lon, level);
        tl_tx = tl.tx; tl_ty = tl.ty;
        br_tx = br.tx; br_ty = br.ty;
        
        LL _x, _y;
        tl.get_x_y(_x, _y);
        if(_x != 0) tl_tx++;
        if(_y != 0) tl_ty++;
        
        br.get_x_y(_x, _y);
        if(_x != tile_l - 1) br_tx--;
        if(_y != tile_l - 1) br_ty--;
    }
    
    Data_Converter::~Data_Converter()
    {
    }
    
    bool Data_Converter::check(const pugi::xml_node &elem, const pugi::xml_node &conf)
    {
        // name should be check in advance
        
        // attributes
        for(auto &attr_conf : conf.attributes())
        {
            if(strcmp(elem.attribute(attr_conf.name()).value(), attr_conf.value()) != 0)
            {
                return false;
            }
        }
        
        // tags
        for(auto &child_conf : conf.children())
        {
            bool flag = false;
            for(auto &child_elem : elem.children(child_conf.name()))
            {
                if(check(child_elem, child_conf))
                {
                    flag = true;
                    break;
                }
            }
            if(!flag) return false;
        }
        
        return true;
    }
    
    pugi::xml_node Data_Converter::get_layer(pugi::xml_node &elem)
    {
        auto layers = this->conf.child("layers");
        pugi::xml_node ret;
        bool force = false;

        for(auto &layer : layers.children())
        {
            for(auto &filter : layer.children(elem.name()))
            {
                if(check(elem, filter))
                {
                    if(!force)
                    {
                        ret = layer;
                        force = layer.attribute("force_").as_bool();
                    }
                    else if(layer.attribute("force_").as_bool())
                    {
                        ret = layer;
                    }
                    break;
                }
            }
        }
        
        auto elem_layer = elem.find_child_by_attribute("tag", "k", "layer");
        if(!force && elem_layer != NULL)
        {
            return layers.find_child_by_attribute("layer", "layer", elem_layer.attribute("v").as_string());
        }
        if(ret != NULL)
        {
            return ret;
        }
        else
        {
            return layers.find_child_by_attribute("layer", "layer", "0");
        }
    }
    
    pugi::xml_node Data_Converter::get_method(pugi::xml_node &elem)
    {
        auto methods = this->conf.child("elements");
        
        for(auto &method : methods.children())
        {
            for(auto &filter : method.children(elem.name()))
            {
                if(check(elem, filter))
                {
                    return method;
                }
            }
        }
        
        throw "No corresponding method.";
    }
    
    void Data_Converter::convert_way(pugi::xml_node &way, pugi::xml_node &layer, pugi::xml_node &method)
    {
        std::vector<cv::Point2i> pts;
        std::set<std::pair<LL, LL> > txys;
        LL tx0 = -1, ty0 = -1;
        LL ltx = -1, lty = -1;
        LL tl_tx = -1, tl_ty = -1, br_tx = -1, br_ty = -1;
        
        for(auto &nd : way.children("nd"))
        {
            auto &node = link[getName(nd)];
            Coord pt(node.attribute("lat").as_double(), node.attribute("lon").as_double(), level);
            LL x, y;
            pt.get_x_y(x, y, tx0, ty0);
            if(tx0 == -1 || ty0 == -1)
            {
                tx0 = pt.tx;
                ty0 = pt.ty;
            }
            int ix = int(x), iy = int(y);
            pts.push_back(cv::Point2i(ix, iy));
            
            if(ltx != -1 && lty != -1)
            {
                LL tx_s = std::min(ltx, pt.tx), tx_e = std::max(ltx, pt.tx);
                LL ty_s = std::min(lty, pt.ty), ty_e = std::max(lty, pt.ty);
                for(LL itx = tx_s; itx <= tx_e; itx++)
                {
                    for(LL ity = ty_s; ity <= ty_e; ity++) txys.insert(std::make_pair(itx, ity));
                }
            }
            
            if(tl_tx == -1)
            {
                tl_tx = br_tx = pt.tx;
                tl_ty = br_ty = pt.ty;
            }
            else
            {
                tl_tx = std::min(tl_tx, pt.tx);
                tl_ty = std::min(tl_ty, pt.ty);
                br_tx = std::max(br_tx, pt.tx);
                br_ty = std::max(br_ty, pt.ty);
            }
            
            ltx = pt.tx; lty = pt.ty;
        }
        
        bool closed = false;
        if(pts[0] == pts[int(pts.size()) - 1]) closed = true;
        
        if(closed)
        {
            for(LL itx = tl_tx; itx <= br_tx; itx++)
            {
                for(LL ity = tl_ty; ity <= br_ty; ity++) txys.insert(std::make_pair(itx, ity));
            }
        }
        
        double epsilon = 1e-10;
        if(level < 17) epsilon *= pow(5.5, 17 - level);
        //std::cerr << level << " " << epsilon << std::endl;
        //std::cerr << "Before approx: " << pts.size() << " ";
        //if(closed){for(auto p : pts) std::cerr << p << " "; std::cerr << std::endl;}
        //cv::approxPolyDP(pts, pts, epsilon, closed);
        //if(closed){for(auto p : pts) std::cerr << p << " "; std::cerr << std::endl;}
        //std::cerr << "After approx: " << pts.size() << std::endl;
        
        int layer_id = layer.attribute("layer").as_int();
        for(const auto &txy : txys)
        {
            std::stringstream ss;
            //ss << "way" << " " << way.attribute("id").value() << " ";
            for(const auto &attr : method.attributes())
            {
                ss << attr.name() << " " << attr.value() << " ";
            }
            ss << std::endl;
            //ss << pts.size() << " ";
            
            LL tx = txy.first, ty = txy.second;
            LL biasx = (tx0 - tx) * tile_l * sampling_factor * retina_factor, biasy = (ty0 - ty) * tile_l * sampling_factor * retina_factor;
            
            for(const auto &pt : pts)
            {
                ss << pt.x + biasx << " " << pt.y + biasy << " ";
            }
            if(closed) ss << pts[0].x + biasx << " " << pts[0].y + biasy << " ";
            ss << std::endl;

            tiles[tx][ty][layer_id].push_back(ss.str());
        }
    }
    
    void Data_Converter::convert(const std::string &path, elem_layers_t &elem_layers, elem_plot_methods_t &elem_plot_methods)
    {
        for(const auto &name : ac)
        {
            pugi::xml_node &elem = link[name];
            
            if(!elem_layers.count(name))
            {
                elem_layers[name] = get_layer(elem);
                elem_plot_methods[name] = get_method(elem);
            }
            pugi::xml_node layer = elem_layers[name];
            pugi::xml_node plot_method = elem_plot_methods[name];
            
            if(strcmp(elem.name(), "way") == 0)
            {
                convert_way(elem, layer, plot_method);
            }
        }
        
        std::map<int, std::string> layer_attrs;
        auto layers = this->conf.child("layers");
        
        for(auto &layer : layers.children())
        {
            std::stringstream ss;
            for(auto &attr : layer.attributes())
            {
                ss << attr.name() << " " << attr.value() << " ";
            }
            ss << std::endl;
            
            int layer_id = layer.attribute("layer").as_int();
            layer_attrs[layer_id] = ss.str();
        }
        
//        std::stringstream ss_bg;
//        auto bg = layers.find_child_by_attribute("layer", "layer", "background");
//        for(auto attri : bg.attributes())
//        {
//            ss_bg << attri.name() << " " << attri.value() << " ";
//        }
//        ss_bg << std::endl;
        
        std::ofstream dt_list(path + "dt_list", std::ofstream::app);
        
        for(LL tx = tl_tx; tx <= br_tx; tx++)
        {
            for(LL ty = tl_ty; ty <= br_ty; ty++)
            {
                std::stringstream filename;
                filename << path << tx << "_" << ty << "_" << level << ".dt";
                std::ofstream out(filename.str().c_str(), std::ofstream::out);
                dt_list << filename.str() << std::endl;
                
                //out << ss_bg.str();
                out << bg_generator.get(level, tx, ty);
                
                auto  &tile = tiles[tx][ty];
                std::vector<int> layer_ids;
                
                for(auto it = tile.begin(); it != tile.end(); ++it) layer_ids.push_back(it->first);
                std::sort(layer_ids.begin(), layer_ids.end());
                
                for(auto id : layer_ids)
                {
                    auto &elems = tile[id];
                    out << layer_attrs[id];
                    
                    out << elems.size() << std::endl;
                    for(const auto &elem : elems)
                    {
                        out << elem;
                    }
                }
                
                out.close();
            }
        }
        
        dt_list.close();
    }
    
} // namespace YuxinMap