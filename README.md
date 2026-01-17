# 一个基于QT开发的聊天客户端
可发送图片，文字，支持文件传输、断点续传<br>
可进行图片预览、obj模型查看<br>
新增AI助手，使用llama.cpp，基于本地运行的Qwen3大模型，实现可思考、可调用工具的AI助手<br>
服务器搭建在linux，支持消息存储，文件存储<br>

![image](https://github.com/HYIND/ChataApp/blob/master/images/image1.png?raw=true)

##


## AI助手

基础对话
![image](https://github.com/HYIND/ChataApp/blob/master/images/image2.png?raw=true)

### 工具调用1

定义简单工具供调用查询当前用户信息<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image3.png?raw=true)
<br><br><br>工具查询结果并回复<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image4.png?raw=true)


### 工具调用2
定义工具使ai具备发送消息的功能<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image5.png?raw=true)
![image](https://github.com/HYIND/ChataApp/blob/master/images/image6.png?raw=true)
<br><br><br>自动帮助用户发送消息<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image7.png?raw=true)


### 聊天记录总结
可以调用AI助手帮助总结会话的内容<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image8.png?raw=true)
查看AI总结的结果<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image9.png?raw=true)

## OCR集成
对应静图，自动调用OCR，可覆盖显示在文字对应区域<br>
![image](https://github.com/HYIND/ChataApp/blob/master/images/image10.png?raw=true)