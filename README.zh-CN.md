[![EN](https://img.shields.io/badge/English-222222?style=flat-square)](README.md)
[![中文](https://img.shields.io/badge/简体中文-0088FF?style=flat-square)](README.zh-CN.md)

# 安卓设备文件备份~~大师~~工具

本工具用于将安卓手机/设备上指定的文件夹和文件备份到Windows电脑上，适用于需要长期多次不定时将手机中特定文件夹和文件（如照片，音乐，视频，个人资料文件等）备份到电脑上的需求：

- **增量备份**：比对手机和电脑上指定的文件夹和文件，若有新增文件，或文件被更新，则进行备份，否则将被忽略
- **手机需启用ADB调试**：使用谷歌公司提供的官方工具ADB（Android SDK Platform-Tools中提供）进行备份，手机需启用ADB调试，并用USB数据线连接至Windows电脑
- **针对文件备份**：空文件夹不会进行备份
- **手机无需root**：仅可备份普通权限的文件，无法备份需要root等权限的文件
- **本工具运行环境**：Windows （仅测试了Windows 11）

## 编译开发

本工具需使用Visual Studio 2022或更高版本进行编译开发，VS中必须安装MSVC v143及相关开发工具


## 使用本工具

若不想编译开发，也可直接使用编译好的工具（x64/Release目录下提供了编译好的AdbBackupMaster.exe）。将ADB目录下的三个文件（exe和dll）拷贝到x64/Release目录下，然后双击运行AdbBackupMaster.exe即可。

**注意**：本工具运行依赖Microsoft Visual C++ v14 Redistributable (x64)。若运行工具时提示缺少文件，可安装VCRedist目录下的VC_redist.x64.exe（其版本为14.50.35719），也可去微软官方网站下载安装更新版本。

另：ADB工具的版本可参看ADB目录下的readme.txt文件。

## 工具使用说明

![本地图片](ReadmeImages/1.png "使用说明")

![本地图片](ReadmeImages/2.png "使用说明")

![本地图片](ReadmeImages/3.png "使用说明")
注意：若所选择的备份列表中包含较多的文件夹或子文件夹，Fetch Backup Info时可能需要一些时间。

![本地图片](ReadmeImages/4.png "使用说明")
