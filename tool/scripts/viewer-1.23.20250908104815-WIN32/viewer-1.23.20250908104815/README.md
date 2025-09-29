# 点云查看工具
## 维护
yuyihui@roborock.com

## 主要功能
 - 支持各类点云，图片，轨迹等数据的解析和显示
 - 支持控制点云播放效果，包括逐帧，跳转，变速，倒放等
 - 支持选择点云进行高亮或过滤，并拟合平面
 - 支持使用标定文件进行点云投影
 - 支持显示坐标系平面
 - 支持复合数据的输入，例如轨迹+点云或者轨迹+图片
 - 支持录制 GIF 动态图
 - 支持不同点云文件的对比播放

## 使用方式

### 添加到 Windows 右键菜单
为了方便使用，工具包内提供一个 **rightclick_add_viewer.bat** 和一个**rightclick_delete_viewer.bat** 脚本。
 - **rightclick_add_viewer.bat** 这个脚本会增加以下选项到右键菜单：
    - “**Open with Viewer**” 到文件的右键菜单
    - “**Search with Viewer**” 到文件夹的右键菜单，包含二级菜单
        - “**Search only**”，此选项会搜索当前目录并单独播放支持的文件
        - “**Specify calibs dir**”，此选项会搜索当前目录并单独播放支持的文件，且要求输入标定搜索目录
        - “**Search calibs here**”，此选项会搜索当前目录并单独播放支持的文件，且在当前目录搜索标定
        - “**Search calibs at up level dir**”，此选项会搜索当前目录并单独播放支持的文件，且在上一级目录搜索标定
    - “**Search and group with Viewer**” 到文件夹的右键菜单，包含二级菜单
        - “**Search only**”，此选项会搜索当前目录并按文件夹对数据分组播放
        - “**Specify calibs dir**”，此选项会搜索当前目录并按文件夹对数据分组播放，且要求输入标定搜索目录
        - “**Search calibs here**”，此选项会搜索当前目录并按文件夹对数据分组播放，且在当前目录搜索标定
        - “**Search calibs at up level dir**”，此选项会搜索当前目录并按文件夹对数据分组播放，且在上一级目录搜索标定
 - **rightclick_delete_viewer.bat** 这个脚本会完全删除上述菜单项

这两个脚本需要**以管理员身份运行**，否则会运行失败
如果获得了更新的版本，需要手动再运行一次脚本才能更新右键菜单指向正确的版本。

### 搜索脚本 - seek.bat
 双击后可以粘贴点云根目录路径，程序会递归搜索给定目录，收集所有符合命名格式的文件并单独播放。 脚本会要求两个路径，一个是搜索点云文件的路径，一个是搜索标定文件的路径。第一个要求输入的是点云文件的路径，必选项。第二个要求输入的是搜索标定文件的路径，可选项，可以直接回车跳过。
### 搜索并根据文件夹分组脚本 - seek_and_group.bat
 双击后可以粘贴点云根目录路径，程序会递归搜索给定目录，收集所有符合命名格式的文件并把在一个文件夹内的数据归类在一起播放。 脚本会要求两个路径，一个是搜索点云文件的路径，一个是搜索标定文件的路径。第一个要求输入的是点云文件的路径，必选项。第二个要求输入的是搜索标定文件的路径，可选项，可以直接回车跳过。
### 打开单个文件 - open_file.bat
 双击后可以粘贴点云文件完整路径。 脚本会要求两个路径，一个是搜索点云文件的路径，一个是搜索标定文件的路径。第一个要求输入的是点云文件的路径，必选项。第二个要求输入的是搜索标定文件的路径，可选项，可以直接回车跳过。
### 手动配置 - run.bat
其中包含一些预定义好的命令，使用者可以自行编辑 **run.bat** 脚本，配置播放哪些文件，也可以支持自动搜索模式无法支持的文件类型。配置方式见后续章节 
run脚本配置
 脚本中存在两个变量，**groupid**为组别的标志，**cmd**为输入到程序中的命令集合。你需要做事情是，把你想要一起播放的文件，放在一个组别中即可。
 以windows脚本为例，标准的一个组别是：
 ```
   // 创建一个新组
   set cmd=!cmd! -group=!groupid! 
   // 往组中增加两个文件，这两个文件将会一起播放
   set cmd=!cmd! -file=C:\Users\me\Downloads\ocy-1.1-1.2-2\PointCloud_flood_3326624.ply -type=50 -pack=0:3
   set cmd=!cmd! -file=C:\Users\me\Downloads\ocy-1.1-1.2-2\PointCloud_flood_34351.ply -type=50 -pack=0:3
   // 结束这个组
   set /a groupid+=1 
```
 ubuntu脚本类似。可以有多个组，按顺序堆放即可

 要增加一个标定文件，需要在相应的组内，制定：
 ```
    -rtfile=/path/to/tof0.bin
    -rtfile=/path/to/tof1.bin
 ```
 要增加rgb的内参（用于去畸变）和外参（用于tof-cam陪准），需要在相应的组内，制定：
 ```
    -rgbint=/path/to/内参.bin
    -rgbext=/path/to/外参.bin
 ```
 指定标定文件后，可以在播放时在控制面板选择是否使用标定文件进行点云投影
 要增加一个文件，需要指定：
 ```
   -file, 文件完整路径
   -type, 文件类型
   -pack, 解析文件所需的额外参数，用英文冒号分隔
   -cs,   点云对应的坐标系，可选 tof, body, body:gndrect，可选项，默认为tof
 ```
 例如 -file=C:\Users\me\Downloads\ocy-1.1-1.2-2\PointCloud_flood_3326624.ply -type=50 -pack=0:3， 指定了一个类型为50的文件，并且输入了0和3作为辅助解析的额外参数

 目前可选的文件类型，和需要的额外参数包如下：
 
 文件类型|文件说明|额外参数|额外参数示例|一般情况文件名示例
 |:---------:|:---------:|:---------:|:---------:|:---------|
 |4|OMS pcbin|点云宽:点云高|240:90|pc\_flood\_0.bin,pc\_spot\_hdr\_*.bin|
 |5|ChaoFeng pcbin|点云宽:点云高|120:48|pc_\*\_flood.bin,pc_\*\_spot.bin|
 |1001|Bin10，Bin14|无|无|*binId10.log,*binId14.log|
 |1002|TOF标定解密数据|无|无|*.bin.tof\_cali\_decrypt,Rotate.bin,Static.bin,Static\_down.bin,Trans.bin|
 |302|图片序列|文件名格式:时间戳占位:时间戳单位|[ts].png:[ts]:-3|-file为文件夹路径，其下存储相同命名格式的图片，比123456.png。<br>额外参数中的第一个部分为命名格式，时间戳用某个占位符号替代。例如 123456.png对应的格式应该是 [ts].png, somename\_aaa\_bbb\_ccc\_123456\_ddd\_eee\_fff.png  对应的格式应该是 somename\_aaa\_bbb\_ccc\_[ts]\_ddd\_eee\_fff.png。命名格式支持正则表达式，比如somename\_.\*\_[ts].png, 可以匹配somename_aaa\_[ts].png和somename\_bbb\_[ts].png。<br>额外参数中的第二个部分为使用的时间戳占位符，比如上述例子中，为[ts]。<br>额外参数中的第三个部分时间戳的单位，其值代表和秒之间的转换关系。例如，-3代表时间戳*10^-3后单位变成秒，也就是时间戳单位是ms|
 |50|ply点云|SensorId:DataLabel|0:3|*.ply|
 |54|ply点云序列|文件名格式:时间戳占位:时间戳单位|[ts].ply:[ts]:-3|-file为文件夹路径，其下存储相同命名格式的ply，比如123456.ply。额外参数定义同302|
 |56|pcd点云|SensorId:DataLabel|0:3|*.pcd|
 |57|pcd点云序列|文件名格式:时间戳占位:时间戳单位|[ts].pcd:[ts]:-3|-file为文件夹路径，其下存储相同命名格式的pcd，比如123456.pcd。额外参数定义同302|
 |58|ChaoFeng csv点云|点云宽:点云高:类型|120:48:3|RAW_ORIGIN_143652398_0_front_314_335.csv|
 |59|ChaoFeng csv点云序列|文件名格式:时间戳占位:时间戳单位:点云宽:点云高:类型|RAW_ORIGIN_[ts]_[0-9]+_front_[0-9]+_[0-9]+.csv:[ts]:-3:120:48:3|-file为文件夹路径，其下存储相同命名格式的pcd，比如123456.pcd。额外参数定义参考302|
 |62|线激光 csv 点云|无|无|CloutPoint_xxx.csv, 文件第一行为titile，固定为“loctime,devicetime,Exptime,valid,filtering,x,y,luma,row”|

也可参考脚本 scripts/helpers/func_get_file_cmds.bat 中的设置
### 播放控制
  viewer 运行后，会显示两个窗口，一个是点云显示窗口，一个是控制面板。
  - 鼠标点击点云显示窗口，然后可以使用以下快捷键
    - A/D： 逐帧播放，播放上一帧/下一帧
    - 向左箭头/向右箭头： 降低/增加播放速度
    - 向上箭头/向下箭头： 提升播放速度到最大/降低播放速度到最小
    - N/B： 播放上一个组/播放下一个组
    - "[" / "]": 减小/增大点的尺寸
    - 空格：暂停播放
  - 鼠标点击点云显示窗口，鼠标左键可旋转点云，滚轮可缩放，按住滚轮可拖拽平移点云。
  - 在控制面板，有各个可收起的控制页，请自行探索。

## 更新方式
获取最新版本方法：
 - 永久更新地址：\\\192.168.111.103\软件视觉标定算法\Tools\viewer
 - 向 yuyihui@roborock.com 索取

如果你添加了右键快捷方式，则需要在上一个版本里，以管理员身份运行 **rightclick_delete_viewer.bat** 然后再次运行 **rightclick_add_viewer.bat**

## Bug和需求
如果你发现某个点云文件无法正确解析，或者希望viewer能对特定格式进行支持，或者对viewer的使用有建议，不要犹豫，立刻联系 yuyihui@roborock.com