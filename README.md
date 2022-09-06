##（提示：图片没法加载的话可能需要你挂一下vpn！）

### 基于C++的文档搜索引擎

#### 一. 项目背景

- 在使用`C++`进行开发的时候，我们很可能会用到`Boost`库，这个库采用了类似`STL`的编程范式，但却比`STL`优美清晰得多。
- `Boost`官网其实已经给我们提供了一个搜索引擎，但官网提供的搜索引擎是面向与`Boost`库有关的所有内容（包括简介、组织机构、下载、版本发布等等），**而实际上我们只想关注`Boost`提供的函数。**
- 此项目一开始就把搜索范围局限到`Boost`函数之内，排除很多与`Boost`函数无关的内容，很大程度上简化了我们搜索`Boost`库函数的过程。

<br></br>

#### 二. 效果展示

该项目的前端实现效果如下图所示

- 搜索前

![1.png](https://github.com/ChongbinZhao/cpp-search-engine/blob/master/pic/1.png?raw=true)

<br>

- 搜索后

![2.png](https://github.com/ChongbinZhao/cpp-search-engine/blob/master/pic/2.png?raw=true)

<br></br>

#### 三. 技术栈

**前端部分**

- `html`：对网页的进行布局，负责控件的摆放、背景图片、网页标题、网页图标等。
- `css`：其实就是对HTML页面进行美化，可以控制字体大小、字体样式、控件形状、控件颜色等其他效果。
- `javascript`：用来定义控件的功能，类似于`Qt`的信号和槽，简单来说就是在网页中点击一个控件就可以触发一个函数，这个函数的实现由`javascript`来编写。

<br>

**后端部分**

- `C++`：整个项目是基于`C++`展开的，项目中大量使用到`STL`提供的数据结构与算法。
- `http`：搜索引擎服务端的搭建以及客户端/服务端之间数据的传输。
- `jiebacpp`：这是前人封装好的库，项目中调用`jiebacpp`的库函数来对搜索文本进行分词操作。
- `jsoncpp`：服务端会将搜索结果以`Json`的形式发送给浏览器。
- `MySQL`：一个`html`网页可以按照["标题"，"网址"，"正文内容"] 三个部分划分，我们会事先将所有的`html`网页以[“id”，标题"，"网址"，"正文内容"]的格式存入`MySQL`数据库里，在服务端启动时将`html`网页的数据读取出来。
- `redis`：
  - 利用`redis`将搜索结果存储在内存中，若在`key`的过期时间内搜索且命中缓存，则可以大幅提高搜索速度。
  - 为了避免频繁建立`redis`连接所带来的消耗，我们会事先将初始化好的`redis`连接保存在一个链表里，即`redis`连接池。
  - 我们还将`redis`连接d的调用封装成了一个[`RAII`类](https://baike.baidu.com/item/RAII/7042675)，等有客户端有需求时从`redis`连接池取出一个连接，等搜索过程结束后又将这个`redis`连接放回连接池里。
- `锁机制与信号量机制`：
  - 由于服务端是可以接受多个客户端连接的，也就是说内存中的`redis`连接池由所有客户端共同维护
  - 为了防止`redis`连接池发生脏读、脏写的现象，每次从`redis`连接池取出和放回`redis`连接都需要加锁和解锁。
  - 由于`redis`连接池中`redis`连接的数量有限，所以获取`redis`连接前后还要进行信号量的`PV`操作（`wait()`和`post()`）。
- `TD-IDF算法`：
  - 这个算法是用来描述一个关键词对一个文档的重要程度。
  - 通常来说，一个关键词在某个文档中的`TD-IDF`值越大，那么这个关键词在这个文档中的重要程度也就越高。
  - 搜索结果的排序顺序由该关键词对每一个网页的`TD-IDF`值的大小来决定。

<br></br>

#### 四. 实现过程

整个`C++`文档搜索引擎的后端逻辑可以大致分为五个模块：

- `http`网络模块
- `HTML`文件预处理模块
- `MySQL`离线存储模块
- 搜索算法模块
- `redis`缓存模块

<br>

##### 1. `http`网络模块

- 服务端的搭建我们可以调用一个叫做[`cpp-httplib`](https://github.com/yhirose/cpp-httplib)的`http`开源库，这个开源仅包含一个头文件，但是代码多达8000行。
- `cpp-httplib`服务端采用的是`IO`多路复用模型是`select`（通常`epoll`的性能更好 ），以线程池的方式来处理客户端连接，主要有`Server`，`Client`，`Request`和`Response`这几种类，项目中只用到`Server`类。
- 服务端Server工作流程如下图所示![3.png](https://github.com/ChongbinZhao/cpp-search-engine/blob/master/pic/3.png?raw=true)

- 主要代码：

  ```c++
  /*	回调函数：项目中服务端接收到浏览器发来的GET请求后会调用这个函数	*/
  void GetWebData(const httplib::Request& req,httplib::Response& resp){
      //  浏览器将搜索框里的内容通过参数query发送给服务器
      if(!req.has_param("query")){
          resp.set_content("Empty query!","text/plain;charset=utf-8");
          return ;
      }
      
      //  获取参数值
      string query = req.get_param_value("query");
      string results;
      searchf.Search(query,&results);
      resp.set_content(results,"application/json;charset=utf-8");
      
      cout<< "The size of result is : "<< results.size()<<endl<<endl;
  }
  
  /*	Server实现	*/
  using namespace httplib;
  Server server;
  // 设置前端文件的根目录：root = "./WWW"
  ret = server.set_base_dir(root.c_str());
  // 建立回调函数与网址的映射
  server.Get("/searcher",GetWebData);
  // 建立监听,主进程会阻塞在listen这里
  ret = server.listen( "localhost", port);<br>
  ```

<br>

##### 2. `HTML`文件预处理模块

- `Boost`官方提供了所有`Boost`函数的`HTML`源文件，我们可以先从官网下载压缩包，整个压缩包一共包含5827个有效`HTML`，每个`HTML`文件可以按照["标题"，"网址"，"正文内容"] 三个部分来划分。
- 在服务端启动之前，我们将所有的`HTML`文件以[“id”，标题"，"网址"，"正文内容"]的格式存储到`MySQL`数据表里，表结构如下图所示

![4.png](https://github.com/ChongbinZhao/cpp-search-engine/blob/master/pic/4.png?raw=true)

<br>

##### 3. `MySQL`离线存储模块

- **`C++`建立`MySQL`连接**

  ```c++
  /*	C++调用MySQL需要包含的头文件	*/
  #include <mysql/mysql.h>
  
  /*	建立MySQL连接	*/
  MYSQL* con = mysql_init(NULL);
  con = mysql_real_connect(con, "localhost", username.c_str(), password.c_str(), databaseName.c_str(), 3306, NULL, 0);
  ```

- **向`MySQL`数据表插入数据**

  - 如果直接执行`mysql_query(con, sql_insert.c_str())`查询语句，这样其实就相当于在命令行手动键入一条`sql_insert`查询语句一样。这样做理论好像没错，但实际上我们插入的内容是一个很长的字符串，当字符过多时，就可能出现字符转义的问题，使得词法分析器无法识别错误。

    ```c++
    /*	比如我想要插入字符串为" abcde'dfa' "	*/
    //执行查询语句
    INSERT INTO HTML_TABLE VALUES(id,title,url,"abcde'dfa'");
    
    //由于 " ' "发生了转义，所以插入失败
    ```

  - 字符转义的问题其实可以通过`MySQL`的预处理语句(`Prepared Statement`)来解决。

    ```c++
    MYSQL_STMT *stmt = mysql_stmt_init(con);
    string sql_insert = "INSERT INTO HTML_TABLE VALUES(default, ?, ?, ?)";
    
    YSQL_BIND params[3];
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)doc_info._title.data();
    params[0].buffer_length = doc_info._title.size();
    
    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)doc_info._url.data();
    params[1].buffer_length = doc_info._url.size();
    
    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = (char*)doc_info._content.data();
    params[2].buffer_length = doc_info._content.size();
    
    mysql_stmt_bind_param(stmt, params);
    mysql_stmt_execute(stmt);
    ```

    <br>

##### 4. 搜索模块

- **建立正排索引和倒排索引**

  ```c++
  /*	正排索引指的是通过 HTML网页的id 获得 HTML网页的内容	*/
  struct frontIdx{
  	int64_t _docId;
  	string  _title;
  	string  _url;
  	string  _content;
  };
  
  /*	倒排索引指的是通过网页内容的某个 关键词 推导出 HTML网页的id和TF-IDF权值	*/
  struct backwardIdx{
  	int64_t _docId;
  	double  _TF;	//词频
  	double  _IDF;	//逆文档频率
  	double  _weight;//TF-IDF值
  	string  _word;	//关键词
  };
  ```

- **索引存储结构**

  ```c++
  //  正排索引
  vector<frontIdx> forward_index;
  //  倒排索引（哈希表）
  unordered_map<string,vector<backwardIdx> > inverted_index;
  ```

- **`TD-IDF`算法**

  - 该算法有两层意思，一层是"词频"（`Term Frequency`，缩写为`TF`），另一层是"逆文档频率"（`Inverse Document Frequency`，缩写为`IDF`）

  - 搜索得到的`HTML`网页会根据`TF-IDF`值进行降序排序，然后封装成`Json`格式发送给浏览器(客户端)

  - `TF`计算公式如下
    $$
    词频(TF)=\frac{某个关键词在该文档中出现的次数}{该文档的总词数}
    $$

  - `IDF`计算公式如下
    $$
    逆文档频率(IDF)=\log(\frac{总的文档数}{包含该关键词的文档数+1})
    $$

  - `TF-IDF`的值其实就是两个词的乘积，即`TF-IDF = TF * IDF`。

- **搜索过程原理图**![5.png](https://github.com/ChongbinZhao/cpp-search-engine/blob/master/pic/5.png?raw=true)



<br>

##### 5. `redis`缓存模块

- **`redis`接口**

  - 在项目中主要用到的`redis`接口都被封装到了`redis.hpp`头文件里，要调用的话得`include`这个头文件。
  - 项目中涉及到得`redis`接口：
    - `connect`：建立`redis`连接  。
    - `select`：选择`redis`数据库（默认情况下`redis`有16个数据库）。
    - `set`：设置`key-value`键值对。
    - `get`：根据`key`值获取`value。`
    - `expire`：设置过期时间（项目中对每个`key`都设置30分钟的过期时间，如果超过30分钟没有被访问就删除这个`key`）。
    - `close`：关闭`redis`连接。

- **`redis`连接池**

  - 建立`redis`连接需要消耗一定的时间和空间资源

  - 如果每调用一次查询就要就要建立一次`redis`连接，当这样的操作过于频繁时，就会严重影响服务器的性能。

  - 所以在服务端启动之初，我们预先创建好若干个`redis`连接，然后把这些`redis`连接存入一个链表里，即`redis`连接池；当需要查询`redis`缓存时，就从连接池里取一个`redis`连接出来。

    ```c++
    //  for循环创建含有MaxCon个连接的redis连接池
    for(int i=0; i<MaxCon; i++)
    {
           Redis* con = new Redis();
           con->connect(m_ip, m_port);
           RedisConnectionList.push_back(con);
           m_FreeCon++;
    }
    ```

  - 项目中使用使用局部静态变量懒汉模式创建`redis`连接池，主要代码如下：

    ```c++
    //  局部静态变量懒汉模式
    redis_pool* redis_pool::GetInstance()
    {
        static redis_pool ConnectionPool;
        return &ConnectionPool;
    }
    ```

- **`RAII`机制**

  - `RAII`是`Resource Acquisition Is Initialization`（翻译成 “资源获取就是初始化”）的简称，是`C++`语言的一种管理资源、避免泄漏的惯用法。

  - 在项目中，我将`redis`连接的调用封装成一个`connectionRAII`类。

    ```c++
    //  RAII机制调用redis连接
    connectionRAII::connectionRAII(Redis** con, redis_pool* ConnectionPool)
    {   
        *con = ConnectionPool->GetRedisConnection();
        conRAII = *con;
        poolRAII = ConnectionPool;
    }
    
    //  RAII机制析构，redis连接重新放回连接池内
    connectionRAII::~connectionRAII()
    {
        poolRAII->ReleaseRedisConnection(conRAII);
    }
    ```

  - 这样做的目的就是：

    - 当一个`connectionRAII`对象使用完（`redis`连接离开作用域）之后，`connectionRAII`对象的析构函数就会将这个`redis`连接重新放回`redis`连接池里。
    - 通过这样的方式能够很好的管理`redis`连接并且防止内存泄漏。

- **锁和信号量机制**
  - 由于服务端要处理多个客户端请求，所以说`redis`连接池是由所有客户端共同维护的，为了避免多个客户端同时修改`redis`连接池，我们可以利用**锁机制**确保`redis`连接池在同一时刻只能被一个客户端占用。
  - 又由于`redis`连接池的连接资源是有限的，只有预先的定义好的`MaxCon`个，项目中定义好的信号量为`reserve`，对其的`P`、`V`（分别对应`wait()`和`post()`）操作如下：
    - `P`：如果`reserve`的值大于0，则将其减一；若`reserve`的值为0，则挂起执行。
    - `V`：如果有其他进行因为等待`reserve`而挂起，则唤醒；若没有，则将`reserve`值加一。

<br></br>

#### 四. 优化方向

- `TF-IDF`是一种关键字强相关算法，实际上体现不出来上下文结构（后期有机会可以优化一下这一方面）：
  - 比如说搜索框输入`“LOVE YOU”`和`“YOU LOVE”`得到的搜索结果是一样的，因为只是针对`[“YOU”, "LOVE"]`两个关键词进行相关性推荐。
  - 但是项目中加入了字符串序列算法能够判断出`“LOVE YOU”`和`“YOU LOVE”`对应着同一个搜索结果，这样可以减少重复的存储。
- 加入`Simhash`网页去重算法等。

















