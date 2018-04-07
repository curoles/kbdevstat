package kbdevst

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	//"os"
)

type SysfsPs2Kbd struct {
	Interrupts string `json:"interrupts"` // ps2/kbd/interrupts
}

type SysfsPs2 struct {
	Kbd SysfsPs2Kbd `json:"kbd"` // ps2/kbd
}

type SysfsProfile struct {
	Home  string `json:"home"`  // /sys/module/kbdevst

	Ps2 SysfsPs2 `json:"ps2"`
}

/* Reads /sys/module/kbdevst/profile to get Sysfs paths to all files.
 *
 */
func SysfsProfileRead(path string) (*SysfsProfile, error) {
	raw, err := ioutil.ReadFile(path)
	if err != nil {
		fmt.Println(err.Error())
		return nil, err
	}

	var profile SysfsProfile
	json.Unmarshal(raw, &profile)

	return &profile, nil
}
