# kvs
key-value store and a bit like redis(一个山塞redis项目)

当初只是想玩下数据结构，没想太多，突然就开始写了。

* 已实现的功能
  * set key val
  * get key
  * del key
  * append key val
  
* 将要实现的功能
  * 各种list类型静态方法相关命令, 比如:lpush hello 1 2 3 4 5
  * 各种string类型方法相关命令
  * 超时机制
  * 排序map(skip list or rbtree)
  * bugfix

从一开始kvs就是一个玩具，如果你也想玩下kvs，但是不想从网络层和协议层开始写，可以fork kvs的代码继续写。
从客房端传过来的命令比如，push hello 1 2 3，就会存入如下结构体的argv成员变量里，argc是参数个数，需要注意下argv->ptr
指向kvs_buf_t成员, 该类型的->p成员才是大家熟悉的c语言字符串。遍历下argv成员，就可以取得这些命令。
```c
struct kvs_client_t {
    kvs_buf_t      rbuf;
    int            fd;
    int            nargs;
    int            nhead;

    char           wbuf[KVS_OUTPUT_SIZE]; //small content values
    int            wpos;
    kvs_obj_t    **argv;
    int            argc;

    unsigned       flags;
    kvs_command_t *command;
};
```
其中
```c
static const kvs_command_t cmds[] = {
        {"set",   kvs_command_set,   3},
        {"get",   kvs_command_get,   2},
        {"del",   kvs_command_del,   2},
        {"lpush", kvs_command_lpush, 3},
        {"rpush", kvs_command_rpush, 3},
        {"lange", kvs_command_lange, 3},
        {"llen",  kvs_command_llen,  2},
        {"lpop",  kvs_command_lpop,  2},
        {"rpop",  kvs_command_rpop,  2},
        {"lterm", kvs_command_lterm, 2},
        {"append", kvs_command_append,3},
        {NULL, NULL, 0},
    };
```
是命令及其回调函数，实现一个新命令，string (kvs_buf_t) list(kvs_list_t) 范围内的，只要注册命令再写回调函数就可以了。如果想写个新的数据结构，
可以先修改下kvs_obj_t 注册新的类型。重复上面的动作。

## 最后:接下来一段时间内
想玩的时候才会commit下代码，也许两个月才会提交下，也许再也不commit。也许某个月的时间全部写完。
