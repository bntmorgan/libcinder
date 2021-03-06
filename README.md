# Welcome to libpickup

Libpickup is a C shared library to build tinder applications. It comes with a command line interface and a graphical user interface built on GTK3.

## cli preview

```
$ xml -help
Usage: xml updates
       xml matches list
       xml matches { print | images | gallery | update } MATCH
       xml matches message MATCH MESSAGE
       xml recs { list | scan }
       xml recs { print | like | dislike | images | gallery } REC
       xml user { credentials | auth | logout }
Options := { -h[help] | -v[erbose] | -q[uiet] |
             -d[ebug] | -list-possible-arguments }
```
```
$ xml r
59b05f1b6a8e1fa34f9febc0 Estelle
59a2ef9ca57fdfb607fdd315 Laura
59ae90ca73bf68e32b723f27 Sarah
596fbfbf59d1770752abaf80 Pauline
59a1b231bdc82efc1fef3c05 Victoria
59ac451cc98b800d2c779694 Lili
59b0fc3ca79c94e726b788d1 Célia
59a9aacb23ae8efb02203a9f Clara
599825d8945d42a05cff7040 Cé
59a1a47bd3a0b3292003e9af Clémence
59ada84673bf68e32b71a628 Agnes
```

## gui preview

For now, users shall log in with the command line interface and then start the gui

```
$ xml u a
$ gui
```

<a href="http://30000-makina.com/xml-gui.gif"><img alt="Gif video" src="http://30000-makina.com/xml-gui.gif"/></a>

[![Watch the video](http://30000-makina.com/gui-play.png)](http://30000-makina.com/toto.webm)

Recommendations

![alt text](http://30000-makina.com/gui.png)

Matches

![alt text](http://30000-makina.com/gui2.png)

Messages

![alt text](http://30000-makina.com/gui3.png)

It is realeased with a sample application and a complete command line interface
!

## Lib dependecies:
  - webkit2gtk-4.0
  - yajl
  - libcurl
  - sqlite3
  - gtk+-3.0
  
## Programs dependecies:
  - feh

## You can check your dependencies with :

```
$ pkg-config <pkg-name>
$ echo $? # 0 OK, 1 KO
```

## How to use it (cli or sample) :

  * Create the image cache directory in ~/.cache/pickup/img
  
  ```
  $ mkdir -p ~/.cache/pickup/img
  ```

  * Create the sqlite3 database in ~/.config/pickup/pickup.db with the provided
    schema
    
 ```
 $ sqlite3 .cache/pickup/pickup.db
 sqlite> .read SCHEMA.sql
 ```

  * The lazy boy may do:

```
$ LD_LIBRARY_PATH=binary/libpickup/:binary/liboauth2webkit/ binary/cli/xml --help
```

  * The power user may add in its shell rc
    binary/cli to its PATH and libs to LD_LIBRARY_PATH environment variables :

```
export PATH=$PATH:<right_dir>/binary/cli
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<right_dir>/binary/libpickup:<right_dir>/binary/liboauth2webkit
```
