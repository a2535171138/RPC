rocket.xml存储服务端日志参数  
rocket_client.xml存储客户端日志参数  
  
创建一个log文件夹  
运行test_rpc_server，会创建一个日志文件，循环往里写入日志内容  
运行test_rpc_server，会创建2个日志文件，循环往client日志里写入日志内容  
连接成功，创建test_rpc_server_app_20240822_log.0，并写入以下信息  
[DEBUG]	[24-08-22 08:46:26.451]	[45544:45550]	[99998888]	[makeOrder]	[testcases/test_rpc_server.cc:32]	start sleep 5s  
[DEBUG]	[24-08-22 08:46:31.452]	[45544:45550]	[99998888]	[makeOrder]	[testcases/test_rpc_server.cc:34]	end sleep 5s  
[DEBUG]	[24-08-22 08:46:31.452]	[45544:45550]	[99998888]	[makeOrder]	[testcases/test_rpc_server.cc:44]	call makeOrder success
