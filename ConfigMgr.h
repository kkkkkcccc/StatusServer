#pragma once
#include"const.h"
struct SectionInfo {
	SectionInfo() {}
	~SectionInfo() {
		_section_datas.clear();
	}

	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}

	SectionInfo& operator = (const SectionInfo& src) {
		if (&src == this) {
			return *this;
		}

		this->_section_datas = src._section_datas;
		return *this;
	}

	std::map<std::string, std::string> _section_datas;

	std::string  operator[](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		// 这里可以添加一些边界检查  
		return _section_datas[key];
	}
};


class ConfigMgr
{
public:
	~ConfigMgr() {
		_config_map.clear();
	}
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}

	static ConfigMgr& Inst() {
		static ConfigMgr cfg_mgr;	//局部静态变量生命周期与进程同步，可见范围只见于局部作用域（C++11以上）
		return cfg_mgr;		//多线程访问时，只初始化一次，多次访问使用同一变量===单例模式（线程安全）
	}

	ConfigMgr& operator=(const ConfigMgr& src) {
		if (&src == this) {
			return *this;
		}

		this->_config_map = src._config_map;
	};

	ConfigMgr(const ConfigMgr& src) {
		this->_config_map = src._config_map;
	}

	
private:
	ConfigMgr();
	// 存储section和key-value对的map  
	std::map<std::string, SectionInfo> _config_map;
};
