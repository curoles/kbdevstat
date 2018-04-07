package main


import (
	"log"
	"encoding/json"

	"kbdevstd/kbdevst"
)

func main() {
	log.Println("START")

	profile, err := kbdevst.SysfsProfileRead("./profile.json")
	if err != nil {
		log.Println("can't read profile: ", err.Error())
	}

	profileBytes, err := json.Marshal(profile)
	if err != nil {
		log.Println("can't marshal struct: ", err.Error())
	}
	log.Println(string(profileBytes))

	db := kbdevst.InfluxDbStorage{}

	err = db.Connect(/*udp=*/true)
	if err != nil {
		log.Println("can't connect to DB, error: ", err)
	}

	kbdevst.EventHandlerInstall(kbdevst.EventIdPs2KbdInterrupts, db.StoreEvent)

	err = kbdevst.HandleEvent(kbdevst.EventIdPs2KbdInterrupts, 777, []interface{}{888,100,0})
	if err != nil {
		log.Println("can't handle event: ", err.Error())
	}

	log.Println("EXIT. THE END.")
}
