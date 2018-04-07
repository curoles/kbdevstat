package kbdevst

import (
	"errors"
	"fmt"
	"strconv"
)

const (
    EventIdPs2KbdInterrupts = iota

    EventIdEnd = iota
    EventIdSize = EventIdEnd
)

type EventHandler = func(eventId uint, timestamp uint64, val []interface{}) (error)

var eventHandlers [EventIdSize]EventHandler

func EventHandlerInstall(eventId uint, handler EventHandler) (error) {
	if (eventId >= EventIdEnd) {
		fmt.Println("bad eventId=%d", eventId)
		return errors.New("bad eventId")
	}

	eventHandlers[eventId] = handler

	return nil
}

func HandleEvent(eventId uint, timestamp uint64, val []interface{}) (error) {
        if (eventId >= EventIdEnd) {
		return errors.New("bad eventId="+strconv.Itoa(int(eventId)))
	}

	handle := eventHandlers[eventId]

	if handle == nil {
		return errors.New("no event handle installed eventId="+strconv.Itoa(int(eventId)))
	}

	err := handle(eventId, timestamp, val)

	return err
}
