## 安装

通过 autogen.sh 脚本自动生成，该脚本完成以下工作：

- 下载并解压依赖 `mongo-c-driver-1.18.0`；
- 下载并解压依赖 `json-c-0.12`；
- 将 json_compilation_error.patch 应用到 `json-c`；

## FAQ

`mongo-c-driver` 兼容性问题，[ref](https://www.mongodb.com/docs/drivers/c/#compatibility)，1.18.0 版本是支持 MongoDB5.0 的最低版本。

