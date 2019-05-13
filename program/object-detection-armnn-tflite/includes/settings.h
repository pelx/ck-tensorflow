/*
 * Copyright (c) 2018 cTuning foundation.
 * See CK COPYRIGHT.txt for copyright details.
 *
 * SPDX-License-Identifier: BSD-3-Clause.
 * See CK LICENSE.txt for licensing details.
 */

#ifndef DETECT_SETTINGS_H
#define DETECT_SETTINGS_H

#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#if __cplusplus < 201703L // If the version of C++ is less than 17
    #include <experimental/filesystem>
#else
    #include <filesystem>
#endif


template<char delimiter>
class WordDelimitedBy : public std::string {
};

template<char delimiter>
std::istream &operator>>(std::istream &is, WordDelimitedBy<delimiter> &output) {
    std::getline(is, output, delimiter);
    return is;
}
struct FileInfo {
    std::string name;
    int width;
    int height;
};

bool get_yes_no(std::string) ;
bool get_yes_no(char *);
std::string join_paths(std::string, std::string);
std::string str_to_lower(std::string);
std::string str_to_lower(char *);
std::vector<std::string> *readClassesFile(std::string);
float *readAnchorsFile(std::string, int&);
void make_dir(std::string);

inline std::string alter_str(std::string a, std::string b) { return a != "" ? a: b; };
inline std::string alter_str(char *a, std::string b) { return a != nullptr ? a: b; };


class Settings {
public:
    Settings() {

        // Load settings
        std::ifstream settings_file("env.ini");
        if (!settings_file)
            throw "Unable to open 'env.ini' file";
        _verbose = get_yes_no(getenv("VERBOSE"));
        if (_verbose) std::cout << "Settings from 'env.ini' file:" << std::endl;
        std::map<std::string, std::string> settings_from_file;
        for (std::string s; !getline(settings_file, s).fail();) {
            if (_verbose) std::cout << '\t' << s << std::endl;
            std::istringstream iss(s);
            std::vector<std::string> row((std::istream_iterator<WordDelimitedBy<'='>>(iss)),
                                         std::istream_iterator<WordDelimitedBy<'='>>());
            if (row.size() == 1)
                settings_from_file.emplace(row[0], "");
            else
                settings_from_file.emplace(row[0], row[1]);
        }

        std::string model_dataset_type = settings_from_file["MODEL_DATASET_TYPE"];
        if (model_dataset_type != "coco") {
            throw ("Unsupported model dataset type: " + model_dataset_type);
        }

        std::string nms_type = alter_str(getenv("USE_NMS"), "no");
        if (str_to_lower(nms_type) == "no") {
            _graph_file = std::string(getenv("CK_ENV_TENSORFLOW_MODEL_TFLITE_GRAPH_NO_NMS"));
        } else {
            std::cout << std::endl << "ERROR: Unsupported USE_NMS type - " << nms_type << std::endl;
            exit(-1);
        }
        _graph_file = join_paths(std::string(getenv("CK_ENV_TENSORFLOW_MODEL_ROOT")), _graph_file);

        std::string classes_file = std::string(getenv("CK_ENV_TENSORFLOW_MODEL_ROOT")) + "/" +
                                   getenv("CK_ENV_TENSORFLOW_MODEL_CLASSES");
        _model_classes = *readClassesFile(classes_file);

        std::string anchors_file = std::string(getenv("CK_ENV_TENSORFLOW_MODEL_ROOT")) + "/" +
                                   getenv("CK_ENV_TENSORFLOW_MODEL_ANCHORS");
        _m_anchors = readAnchorsFile(anchors_file, _m_anchors_count);

        _images_dir = settings_from_file["PREPROCESS_OUT_DIR"];
        _images_file = settings_from_file["PREPROCESSED_FILES"];
        _number_of_threads = std::stoi(settings_from_file["NUMBER_OF_PROCESSORS"]);
        _batch_count = std::stoi(settings_from_file["IMAGE_COUNT"]);
        _batch_size = std::stoi(settings_from_file["BATCH_SIZE"]);
        _image_size_height = std::stoi(settings_from_file["MODEL_IMAGE_HEIGHT"]);
        _image_size_width = std::stoi(settings_from_file["MODEL_IMAGE_WIDTH"]);
        _num_channels = std::stoi(settings_from_file["MODEL_IMAGE_CHANNELS"]);
        _correct_background = settings_from_file["MODEL_NEED_BACKGROUND_CORRECTION"] == "True";
        _normalize_img = settings_from_file["MODEL_NORMALIZE_DATA"] == "True";
        _subtract_mean = settings_from_file["MODEL_SUBTRACT_MEAN"] == "True";
        _full_report = settings_from_file["FULL_REPORT"] == "True";
        _detections_out_dir = settings_from_file["DETECTIONS_OUT_DIR"];

        _use_neon = get_yes_no(getenv("USE_NEON"));
        _use_opencl = get_yes_no(getenv("USE_OPENCL"));
        _number_of_threads = std::thread::hardware_concurrency();
        _number_of_threads = _number_of_threads < 1 ? 1 : _number_of_threads;
        _number_of_threads = std::stoi(alter_str(getenv("CK_HOST_CPU_NUMBER_OF_PROCESSORS"), std::to_string(_number_of_threads)));
        _batch_count = std::stoi(alter_str(getenv("CK_BATCH_COUNT"), "1"));
        _batch_size = std::stoi(alter_str(getenv("CK_BATCH_SIZE"), "1"));
        _full_report = get_yes_no(getenv("FULL_REPORT"));

        _m_max_classes_per_detection = std::stoi(alter_str(getenv("MAX_CLASSES_PER_DETECTION"), "1"));
        _m_max_detections = std::stoi(alter_str(getenv("MAX_DETECTIONS"), getenv("CK_ENV_TENSORFLOW_MODEL_MAX_DETECTIONS")));
        _m_max_total_detections = std::max(100, _m_max_detections);
        _m_detections_per_class = std::stoi(alter_str(getenv("DETECTIONS_PER_CLASS"), "100"));
        _m_num_classes = std::stoi(alter_str(getenv("NUM_CLASSES"), getenv("CK_ENV_TENSORFLOW_MODEL_NUM_CLASSES"))) + _correct_background;
        _m_nms_score_threshold = std::stof(alter_str(getenv("NMS_SCORE_THRESHOLD"), getenv("CK_ENV_TENSORFLOW_MODEL_NMS_SCORE_THRESHOLD")));
        _m_nms_iou_threshold = std::stof(alter_str(getenv("NMS_IOU_THRESHOLD"), getenv("CK_ENV_TENSORFLOW_MODEL_NMS_IOU_THRESHOLD")));
        _m_scale_h = std::stof(alter_str(getenv("SCALE_H"), getenv("CK_ENV_TENSORFLOW_MODEL_SCALE_H")));
        _m_scale_w = std::stof(alter_str(getenv("SCALE_W"), getenv("CK_ENV_TENSORFLOW_MODEL_SCALE_W")));
        _m_scale_x = std::stof(alter_str(getenv("SCALE_X"), getenv("CK_ENV_TENSORFLOW_MODEL_SCALE_X")));
        _m_scale_y = std::stof(alter_str(getenv("SCALE_Y"), getenv("CK_ENV_TENSORFLOW_MODEL_SCALE_Y")));

        _d_boxes = new float [_m_anchors_count * 4]();
        _d_scores = new float [_m_anchors_count * _m_num_classes];

        _d_scores_sort_buf = new float[_m_max_total_detections + _m_max_classes_per_detection];
        _d_classes_sort_buf = new int[_m_max_total_detections + _m_max_classes_per_detection];
        _d_boxes_sort_buf = new int[_m_max_total_detections + _m_max_classes_per_detection];
        _d_classes_ids_sort_buf = new int[_m_num_classes];

        // Print settings
        if (_verbose || _full_report) {
            std::cout << "Graph file: " << _graph_file << std::endl;
            std::cout << "Image dir: " << _images_dir << std::endl;
            std::cout << "Image list: " << _images_file << std::endl;
            std::cout << "Image size: " << _image_size_width << "*" << _image_size_height << std::endl;
            std::cout << "Image channels: " << _num_channels << std::endl;
            std::cout << "Result dir: " << _detections_out_dir << std::endl;
            std::cout << "Batch count: " << _batch_count << std::endl;
            std::cout << "Batch size: " << _batch_size << std::endl;
            std::cout << "Normalize: " << _normalize_img << std::endl;
            std::cout << "Subtract mean: " << _subtract_mean << std::endl;
            std::cout << "Use Neon: " << _use_neon << std::endl;
            std::cout << "Use OpenCL: " << _use_opencl << std::endl;
        }

        // Create results dir if none
        make_dir(_detections_out_dir);

        // Load list of images to be processed
        std::ifstream file(_images_file);
        if (!file)
            throw "Unable to open image list file " + _images_file;
        for (std::string s; !getline(file, s).fail();) {
            std::istringstream iss(s);
            std::vector<std::string> row((std::istream_iterator<WordDelimitedBy<';'>>(iss)),
                                         std::istream_iterator<WordDelimitedBy<';'>>());
            FileInfo fileInfo = {row[0], std::stoi(row[1]), std::stoi(row[2])};
            _image_list.emplace_back(fileInfo);
        }

        if (_verbose || _full_report) {
            std::cout << "Image count in file: " << _image_list.size() << std::endl;
        }
    }

    ~Settings() {
        delete _d_boxes;
        delete _d_scores;
        delete _d_scores_sort_buf;
        delete _d_classes_sort_buf;
        delete _d_boxes_sort_buf;
        delete _d_classes_ids_sort_buf;
    }
    const std::vector<FileInfo> &image_list() const { return _image_list; }

    const std::vector<std::string> &model_classes() const { return _model_classes; }

    int batch_count() { return _batch_count; }

    int batch_size() { return _batch_size; }

    int detections_buffer_size() { return _m_max_detections + 1; }

    int get_anchors_count() { return _m_anchors_count; }

    int image_size_height() { return _image_size_height; }

    int image_size_width() { return _image_size_width; }

    int num_channels() { return _num_channels; }

    int number_of_threads() { return _number_of_threads; }

    bool correct_background() { return _correct_background; }

    bool full_report() { return _full_report; }

    bool normalize_img() { return _normalize_img; }

    bool subtract_mean() { return _subtract_mean; }

    bool use_neon() { return _use_neon; }

    bool use_opencl() { return _use_opencl; }

    bool verbose() { return _verbose; };

    int get_max_detections() { return _m_max_detections; };
    void set_max_detections(int i) { _m_max_detections = i;}

    int get_max_total_detections() { return _m_max_total_detections; };

    int get_max_classes_per_detection() { return _m_max_classes_per_detection; };
    void set_max_classes_per_detection(int i) { _m_max_classes_per_detection = i;}

    int get_detections_per_class() { return _m_detections_per_class; };
    void set_detections_per_class(int i) { _m_detections_per_class = i;}

    int get_num_classes() { return _m_num_classes; };
    void set_num_classes(int i) { _m_num_classes = i;}

    float get_nms_score_threshold() { return _m_nms_score_threshold; };
    void set_nms_score_threshold(float i) { _m_nms_score_threshold = i;}

    float get_nms_iou_threshold() { return _m_nms_iou_threshold; };
    void set_nms_iou_threshold(float i) { _m_nms_iou_threshold = i;}

    float get_scale_h() { return _m_scale_h; };
    void set_scale_h(float i) { _m_scale_h = i;}

    float get_scale_w() { return _m_scale_w; };
    void set_scale_w(float i) { _m_scale_w = i;}

    float get_scale_x() { return _m_scale_x; };
    void set_scale_x(float i) { _m_scale_x = i;}

    float get_scale_y() { return _m_scale_y; };
    void set_scale_y(float i) { _m_scale_y = i;}

    float* get_anchors() { return _m_anchors; };
    float* get_boxes_buf() { return _d_boxes; };
    float* get_scores_buf() { return _d_scores; };
    float* get_scores_sorting_buf() { return _d_scores_sort_buf; };

    int* get_classes_sorting_buf() { return _d_classes_sort_buf; }
    int* get_classes_ids_sorting_buf() { return _d_classes_ids_sort_buf; }
    int* get_boxes_sorting_buf() { return _d_boxes_sort_buf; }

    std::string graph_file() { return _graph_file; }

    std::string images_dir() { return _images_dir; }

    std::string detections_out_dir() { return _detections_out_dir; }

private:
    std::string _detections_out_dir;
    std::string _graph_file;
    std::string _images_dir;
    std::string _images_file;

    std::vector<FileInfo> _image_list;
    std::vector<std::string> _model_classes;

    int *_d_classes_sort_buf;
    int *_d_boxes_sort_buf;
    int *_d_classes_ids_sort_buf;
    int _batch_count;
    int _batch_size;
    int _image_size_height;
    int _image_size_width;
    int _num_channels;
    int _number_of_threads;
    int _m_max_classes_per_detection;
    int _m_max_detections;
    int _m_max_total_detections;
    int _m_detections_per_class;
    int _m_num_classes;
    int _m_anchors_count;

    float *_d_boxes;
    float *_d_scores;
    float *_d_scores_sort_buf;
    float *_m_anchors;

    float _m_nms_score_threshold;
    float _m_nms_iou_threshold;
    float _m_scale_h;
    float _m_scale_w;
    float _m_scale_x;
    float _m_scale_y;

    bool _correct_background;
    bool _full_report;
    bool _normalize_img;
    bool _subtract_mean;
    bool _use_neon;
    bool _use_opencl;
    bool _verbose;
};

float *readAnchorsFile(std::string filename, int& size) {
    std::vector<std::string> lines;
    lines.clear();
    std::ifstream file(filename);
    std::string s;
    while (getline(file, s))
        lines.push_back(s);

    size = lines.size();
    float *result = new float[size * 4]();
    for (int i = 0; i < size; i++) {
        int index = i * 4;
        std::stringstream(lines[i]) >> result[index]
                                    >> result[index + 1]
                                    >> result[index + 2]
                                    >> result[index + 3];
    }
    return result;
}

std::vector<std::string> *readClassesFile(std::string filename) {
    std::vector<std::string> *lines = new std::vector<std::string>;
    lines->clear();
    std::ifstream file(filename);
    std::string s;
    while (getline(file, s))
        lines->push_back(s);

    return lines;
}

bool get_yes_no(std::string answer) {
    std::locale loc;
    for (std::string::size_type i=0; i<answer.length(); ++i)
        answer[i] = std::tolower(answer[i],loc);
    if (answer == "1" || answer == "yes" || answer == "on" || answer == "true") return true;
    return false;
}

bool get_yes_no(char *answer) {
    if (answer == nullptr) return false;
    return get_yes_no(std::string(answer));
}

std::string str_to_lower(std::string answer) {
    std::locale loc;
    for (std::string::size_type i=0; i<answer.length(); ++i)
        answer[i] = std::tolower(answer[i],loc);
    return answer;
}

std::string str_to_lower(char *answer) {
    return str_to_lower(std::string(answer));
}

std::string join_paths(std::string path_name, std::string file_name) {
#ifdef _WIN32
    std::string delimiter = "\\";
#else
    std::string delimiter = "/";
#endif
    if (path_name.back()=='\\' || path_name.back()=='/') {
        return path_name + file_name;
    }
    return path_name + delimiter + file_name;
}

void make_dir(std::string path) {
#if __cplusplus < 201703L // If the version of C++ is less than 17
    // It was still in the experimental:: namespace
    namespace fs = std::experimental::filesystem;
#else
    namespace fs = std::filesystem;
#endif
    fs::create_directory(path);
}

#endif //DETECT_SETTINGS_H