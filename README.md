# php-sentinel
## 安装使用
编译安装
```bash
$ phpize
$ ./configure --with-php-config=/usr/local/php/bin/php-config
# 如果需要调试
# ./configure --with-php-config=/usr/local/php/bin/php-config --enable-debug
$ make && make install
```
配置`php.ini`，在其后面追加
```bash
[sentinel]
extension=sentinel.so
sentinel.api_url=https://mengkang.net/sentinel.html
sentinel.api_cache_ttl=120
sentinel.api_cache_file=/tmp/sentinel.rule
sentinel.log_enabled=1
sentinel.log_file=/tmp/sentinel.log
```
- `sentinel.api_url` 限流查询接口，**记得将 https://mengkang.net/sentinel.html 修改你自己的限流 api 地址，结构保持一致**
- `sentinel.api_cache_ttl` 接口查询结果缓存 2 分钟
- `sentinel.api_cache_file` 接口查询缓存路径
- `sentinel.log_enabled` 是否开启用户自定义的方法和函数日志记录，可以用日志处理工具收集比如阿里云 SLS
- `sentinel.log_file` 日志记录路径

## 原理
在`PHP_MINIT`阶段，通过`zend_set_user_opcode_handler`注册在`opcode`运行环节，对用户态的方法和函数（`ZEND_DO_UCALL`）的运行做处理
```c
zend_set_user_opcode_handler(ZEND_DO_UCALL, php_sentinel_call_handler)
```
在`PHP_RINIT`阶段，获取中间件配置。对于缓存时长内的不发起网络请求
```c
php_sentinel_fetch()
```
限流策略闭环
```c
php_sentinel_call_handler
```
当已经已经在限流列表中的方法或者函数进行拦截，同时对通过的方法和函数进行日志记录（当日志功能开启的情况）然后进行类似 sls 上报，然后中心通过分析 sls 日志再决策是否进行限流，通过接口通知到各个 web 服务器。

注册自定义的异常类`SentinelException`，当检测到执行的方法或者函数需要被限流，则抛出该异常
```c
zend_class_entry ce;
INIT_CLASS_ENTRY(ce, "SentinelException", NULL);
php_sentinel_exception_class_entry =  zend_register_internal_class_ex(&ce, zend_ce_exception);
php_sentinel_exception_class_entry->ce_flags |= ZEND_ACC_FINAL;
```
业务层可以对该异常在最外层进行捕获，按照业务协议做标准错误输出。
```php
try{
    // 项目 dispatcher 调度入口
}catch (\SentinelException $exception){
    // 标准的错误输出 可以是 json 可以 html 页面
}
```