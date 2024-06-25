## Systemtap
~~~bash
sudo stap memory_tracker.stp > memory_tracker.log
~~~

## Mysql
~~~bash
sudo apt-get install libmysqlclient-dev
~~~

~~~bash
gcc reader.c -o reader -lmysqlclient
~~~