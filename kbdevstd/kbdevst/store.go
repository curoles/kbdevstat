package kbdevst

import (
	"log"
	"errors"
	"fmt"
	"time"

	"github.com/influxdata/influxdb/client/v2"
)

type EventStorage interface {

	StoreEvent(eventId uint, timestamp uint64, val uint64) (error)
}

const (
	dbName = "kbdevstdb"
	dbUser = "igor"
	dbPassw = "igor"
)

type InfluxDbStorage struct {
	dbClient client.Client
}


// https://github.com/influxdata/influxdb/blob/master/client/README.md
//
func (db *InfluxDbStorage) Connect(udp bool) (error) {
	var err error = nil
	if udp {
		db.dbClient, err = client.NewUDPClient(client.UDPConfig{
			Addr: "localhost:8089",
		})
	} else {
		// Create a new HTTPClient
		db.dbClient, err = client.NewHTTPClient(client.HTTPConfig{
			Addr:     "http://localhost:8086",
			Username: dbUser,
			Password: dbPassw,
		})
	}
	if err != nil {
		log.Println("can't connect to DB, error: ", err)
	} else {
		log.Println("connected to DB")
	}

	return err
}

func (db *InfluxDbStorage) StoreEvent(eventId uint, timestamp uint64, val []interface{}) (error) {
	log.Printf("InfluxDB: store event:%d timestamp:%d val:%v", eventId, timestamp, val)

	// Create a new point batch
	bp, err := client.NewBatchPoints(client.BatchPointsConfig{
		Database:  dbName,
		Precision: "ns",
	})
	if err != nil {
		return err
	}

	//https://elixir.free-electrons.com/linux/v4.2/source/include/linux/timekeeping.h#L52
	//struct timespec current_kernel_time(void);
	//static inline void getnstimeofday(struct timespec *ts)
	timeStamp := time.Unix(/*sec int64*/0, /*nsec int64*/int64(timestamp))

	switch eventId {
	case EventIdPs2KbdInterrupts: err = db.storeEventPs2Kbd(bp, timeStamp, val)
	default:
		return errors.New(fmt.Sprintf("unknown event ID=%d", eventId));
	}

	if err != nil {
		return err
	}

	// Write the batch
	if err := db.dbClient.Write(bp); err != nil {
		return err
	}

	return err
}


func (db *InfluxDbStorage) storeEventPs2Kbd(
	bp client.BatchPoints,
	timestamp time.Time,
	val []interface{},
) (error) {

	const (
	indexInterrupts = 0
	indexScancode   = 1
	indexDirection  = 2
	)

	if len(val) < 3 {
		return errors.New("too few data items for ps2kbd event")
	}

	tags := map[string]string{}/*
		"direction": upDown[val[indexDirection]],
	}*/

	fields := map[string]interface{}{
		"interrupts":   val[indexInterrupts],
		"scancode":     val[indexScancode],
		"direction":    val[indexDirection],
	}

	pt, err := client.NewPoint("ps2kbd", tags, fields, timestamp)
	if err != nil {
		return err
	}
	bp.AddPoint(pt)

	log.Println("point added to ps2kbd measurement")

	return err
}
