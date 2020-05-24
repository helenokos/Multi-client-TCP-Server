# 編譯
* 產生檔案 : server, client
* make : 編譯
* make clean : 清除執行檔
* make clean all : 清除執行檔並編譯

# User name
* 合法 User name 有三個
    * Alice
    * Bill
    * Caesar
* 其餘皆為不存在的使用者

# Server
* 用法
    * 直接執行程式即可，沒有額外功能也不須進行輸入
    * 若要結束程式，需在 terminal 按下 ctrl+c 或 ctrl+z
    * 範例 : ./server
* 程式碼
    * 使用 multi-process
    * 需要共用變數的部份利用 mmap 來實現
    * 虛擬碼 :
        ```
        socket
        bind
        listen
        while ( accept success )
            fork()
            receive loggin information form client
            while (1)
                send unread message to client(s)
                receive message from client
        ``` 

# Client
* 用法
    * 輸入連線資訊
        * IP
        * port : 10000
        * user name
    * 選項
        * chat : 輸入 0
            * input : user_name "msg"
                * 可輸入多個 user_name，需以空格間隔
        * bye : 輸入 1
            * 在離開連線前會載入尚未讀取的訊息
        * load : 輸入 2
            * 載入尚未讀取的訊息
* 範例 : ./client
    > IP : 127.0.0.1  
    > PORT : 10000  
    > USER : Alice  
    > &lt;login success&gt;  
    > \=======================  
    > &lt;User Bill has sent you a message "hw Dead Line" at 23:59PM 2020/5/20&gt;  
    > \=======================  
    > 0 : chat  
    > 1 : bye  
    > 2 : load msg  
    > input : 0  
    > user(s) "msg" : Bill "omg"  
    > \=======================  
    > &lt;no new msg&gt;  
    > \=======================  
    > 0 : chat  
    > 1 : bye  
    > 2 : load msg  
    > input : 1  
    > &lt;loading unread msg before logout...&gt;  
    > \=======================  
    > &lt;no new msg&gt;  
* 程式碼 :
    * 虛擬碼
        ```
        socket
        connect
        send client information to server
        while (1)
            receive unread msg from client
            send message to server
        ```

