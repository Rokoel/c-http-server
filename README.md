# rokoel-website

Compile with gcc:
  execute from ../Core:
  ```bash
    gcc -pthread core.c server.c socket.c secondary.c -o ../Builds/serv -fPIC
  ```

Execute:
  execute from ../Core:
  ```bash
    sudo ../Builds/serv
  ```


WORKS ON LINUX ONLY (socket.h is available only there)

Planning to make Redis main database in the future.

Script that makes creating new routes easier may be created in the future (shell, python or C).

Using [hitch](https://github.com/varnish/hitch) for changing http to https.

---

# To use https with hitch and test sertificate:

## Install hitch:
```bash
  sudo apt get install hitch
```

## Install openssl:
```bash
  sudo apt get install openssl
```

## Generate key and certificate (this is one command):
```bash
  openssl req -newkey rsa:2048 -sha256 -keyout example.com.key -nodes -x509 -days 365 -out example.crt
```
* Answer simple questions (answers doesn't matter for testing purposes)

## Make it one .pem file:
```bash
  cat example.com.key example.crt > example.pem
```

## Run hitch (standard https and http ports):
```bash
  sudo hitch --ssl --backend='[127.0.0.1]:80' --frontend='[*]:443' --user='root' example.pem
```

## Comile with gcc and run server:
* execute from ../Core:
  ```bash
    gcc -pthread core.c server.c socket.c secondary.c -o ../Builds/serv -fPIC
  ```
  ```bash
    sudo ../Builds/serv
  ```

## Everything should work fine. Try to connect with https from browser. Http connections will also work so it is possible to redirect to http.
