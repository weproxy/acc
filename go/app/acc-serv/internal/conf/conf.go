//
// weproxy@foxmail.com 2022/10/20
//

package conf

import (
	_ "embed"
	"encoding/json"
)

//go:embed conf.json
var DEFAULT_CONF []byte

// Conf ...
type Conf struct {
	Iface struct {
		In  string `json:"in,omitempty"`
		Out string `json:"out,omitempty"`
	} `json:"iface,omitempty"`

	Servers []json.RawMessage `json:"server,omitempty"`
}

// ReadConfig ...
func ReadConfig() (*Conf, error) {
	var c = &Conf{}
	if err := json.Unmarshal(DEFAULT_CONF, c); err != nil {
		return nil, err
	}
	return c, nil
}
