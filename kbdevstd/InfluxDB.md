## DB design

measurement: ps2kbd

|           |  field       | field      | tag       |
| --------- | ------------ | ---------- | --------- |
| timestamp | interrupts   | scancode   | direction |


## Create a Database

### Windows: Create a Database

Start DB daemon in a terminal:

```terminal
~/tools/influxdb/influxdb-1.4.3-1/influxd.exe
```

Use InfluxDB CLI tool to create an user and database.
Use Windows "CMD" terminal, do **NOT** use GitBash since it does not
work, the prompt is not shown.
Even then there is a bug, to work around this bug one has to set `HOME` environment
variable to path where `influx.exe` is installed.

```terminal
> set HOME=C:\Users\ilesik\tools\influxdb\influxdb-1.4.3-1
> C:\Users\ilesik\tools\influxdb\influxdb-1.4.3-1\influx.exe
Connected to http://localhost:8086 version 1.4.3
InfluxDB shell version: 1.4.3
> create user igor with password 'igor'
> grant all privileges to igor
> create database kbdevstdb
```

## Working on InfluxDB with Go Client

```terminal
go get github.com/influxdata/influxdb/client/v2
```

## Appendix: influx.bat wrapper on Windows to fix "no history file" bug

Save to `influx.bat` in the same directory as your influx executable
and run the batch file instead:

```
@ECHO OFF
SETLOCAL
SET HOME=%~dp0
"%~dp0\influx.exe" %*
ENDLOCAL
```

Note: `.influx_history` in the `influx.exe` directory.
