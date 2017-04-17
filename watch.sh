#!/bin/sh

CheckProcess(){ 
# 检查输入的参数是否有效 
if [ "$1" = "" ]; 
then   
   return 1 
fi   
#$PROCESS_NUM获取指定进程名的数目，为1返回0，表示正常，不为1返回1，表示有错误，需要重新启动 
PROCESS_NUM=`ps -ef | grep "$1" | grep -v "grep" | wc -l`  
if [ $PROCESS_NUM -eq 1 ]; 
then   
   return 0 
else   
   return 1 
fi
}


while [ 1 ] ; do
   CheckProcess "./build/bin/Picture_Stitch"
    if [ $? = 1 ];
    then
# 杀死所有test进程，可换任意你需要执行的操作 
	killall -9 Picture_Stitch
	exec ./build/bin/Picture_Stitch $1 &  
    fi
	sleep 1
done
