# CoolQ HTTP API 插件

[![License](https://img.shields.io/badge/license-MIT%20License-blue.svg)](https://raw.githubusercontent.com/richardchien/coolq-http-api/master/LICENSE)

通过 HTTP 对酷 Q 的事件进行上报以及接收 HTTP 请求来调用酷 Q 的 C++ 接口，从而可以使用其它语言编写酷 Q 插件。

## 使用方法

直接到 [Releases](https://github.com/richardchien/coolq-http-api/releases) 下载最新的 cpk 文件放到酷 Q 的 app 文件夹，然后启用即可。由于要上报事件、接受调用请求，因此需要所有权限。

注意如果系统中没有装 VC++ 2010 运行库，酷 Q 启动时会报错说插件加载失败，需要去下载 [Microsoft Visual C++ 2010 可再发行组件包 (x86)
](https://www.microsoft.com/zh-CN/download/details.aspx?id=5555) 安装。

启用后插件将开启一个后台线程用来监听 HTTP，默认监听 `0.0.0.0:5700`，首次启用会生成一个默认配置文件，在酷 Q app 文件夹的 `io.github.richardchien.coolqhttpapi` 文件夹中，文件名 `config.cfg`，使用 ini 格式填写。关于配置项的说明，见 [配置文件说明](https://richardchien.github.io/coolq-http-api/#/Configuration)。

此时通过 `http://192.168.1.123:5700/` 即可调用酷 Q 的函数，例如 `http://192.168.1.123:5700/send_private_msg?user_id=123456&message=你好`，具体的 API 列表见 [API 描述](https://richardchien.github.io/coolq-http-api/#/API)。

酷 Q 收到的消息、事件会被 POST 到配置文件中指定的 `post_url`，为空则不上报。上报数据格式见 [上报数据格式](https://richardchien.github.io/coolq-http-api/#/Post)。

停用插件将会关闭 HTTP 线程，再次启用将重新读取配置文件。

## 文档

更多文档，见 [CoolQ HTTP API 插件文档](https://richardchien.github.io/coolq-http-api/)。

## 修改、编译

整个项目目录是一个 VC++ 2010 工程，直接打开 `CoolQHttpApi.vcxproj` 即可修改、编译。我测试用 VS 2017 RC 来编译工程似乎是不能直接编译成功的，因此如果你使用较新的 VS，可能需要同时安装一个 VC++ 2010，在选择工具集的时候选择 VC++ 2010 的工具集。另外，用 VS Code 来编辑也是个不错的办法，然后直接调用 MSBuild 编译（注意这依然需要安装 VC++ 2010），大致命令如下：

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"
C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe CoolQHttpApi.vcxproj /t:Build /p:Configuration=Release
```

工程属性中配置了一个自定义的「后期生成事件」命令，也就是编译成功后执行的脚本，默认情况下什么都不做，你可以在项目根目录加一个文件 `post_build.bat` 在里面进行你需要的操作（比如移动 dll 文件到酷 Q 的 app 文件夹），这个文件被 Git 忽略掉了，因此不会影响 Git 仓库上的内容。

除了与编译无关的 `README.md`、`LICENSE` 等文件使用 UTF-8 编码，其它代码文件、JSON 文件都使用 GBK 编码，以便在中文 Windows 上正常运行。

## Issues

如果使用过程中遇到任何问题，欢迎提 issue。
