//
// weproxy@foxmail.com 2022/10/20
//

package conf

import (
	_ "embed"
	"encoding/json"
	"errors"
	"math/rand"
	"os"
	"regexp"
	"strings"
	"time"
)

////////////////////////////////////////////////////////////////////////////////

//go:embed conf.json
var _DEFAULT_CONF []byte

////////////////////////////////////////////////////////////////////////////////

// Rule ...
type Rule struct {
	Host []string `json:"host,omitempty"`
	Serv []string `json:"serv,omitempty"`
	regx []*regexp.Regexp
}

// CompileRegexp ...
func (m *Rule) CompileRegexp() error {
	if len(m.Serv) == 0 {
		return nil
	}
	m.regx = make([]*regexp.Regexp, len(m.Host))
	for i, s := range m.Host {
		if s != "" && s != "*" && strings.Contains(s, "*") {
			s = strings.ReplaceAll(s, "*", "\\.*")
			if regx, err := regexp.Compile(s); err != nil {
				return err
			} else {
				m.regx[i] = regx
			}
		} else {
			m.regx[i] = nil
		}
	}
	return nil
}

// IsMatch ...
func (m *Rule) IsMatch(s string) bool {
	if len(m.Serv) == 0 {
		return false
	}
	for i, regx := range m.regx {
		if regx != nil {
			if regx.MatchString(s) {
				return true
			}
		} else if m.Host[i] == "*" || m.Host[i] == s {
			return true
		}
	}
	return false
}

// GetServ ..
func (m *Rule) GetServ() string {
	if n := len(m.Serv); n == 0 {
		return ""
	} else if n == 1 {
		return m.Serv[0]
	} else {
		rand.Seed(time.Now().Unix())
		return m.Serv[rand.Intn(n)]
	}
}

////////////////////////////////////////////////////////////////////////////////

// Conf ...
type Conf struct {
	Server struct {
		Auth struct {
			S5 string `json:"s5,omitempty"`
			SS string `json:"ss,omitempty"`
		} `json:"auth,omitempty"`
		DNS  []string            `json:"dns,omitempty"`
		Main []string            `json:"main,omitempty"`
		GEO  map[string][]string `json:"geo,omitempty"`
	} `json:"server,omitempty"`

	DNS   []*Rule `json:"dns,omitempty"`
	Rules []*Rule `json:"rules,omitempty"`
}

// GetMain ...
func (m *Conf) GetMain() string {
	if n := len(m.Server.Main); n == 0 {
		return ""
	} else if n == 1 {
		return m.Server.Main[0]
	} else {
		rand.Seed(time.Now().Unix())
		return m.Server.Main[rand.Intn(n)]
	}
}

// GetDNS ...
func (m *Conf) GetDNS() string {
	if n := len(m.Server.DNS); n == 0 {
		return ""
	} else if n == 1 {
		return m.Server.DNS[0]
	} else {
		rand.Seed(time.Now().Unix())
		return m.Server.DNS[rand.Intn(n)]
	}
}

////////////////////////////////////////////////////////////////////////////////

var (
	// each source (such as Xbox/PS/Switch) has its own rule config
	_confs = make(map[string]*Conf) // srcIP --> *Conf
)

const _DEFAULT_KEY = "default"

// MustGet ...
func MustGet(srcIP string) *Conf {
	if c, ok := _confs[srcIP]; ok {
		return c
	} else if c, ok := _confs[_DEFAULT_KEY]; ok {
		return c
	} else if c, err := ParseBytes(_DEFAULT_KEY, _DEFAULT_CONF); err == nil {
		return c
	}
	return &Conf{}
}

// Parse ...
func Parse(srcIP, js string) (*Conf, error) {
	return ParseBytes(srcIP, []byte(js))
}

// ParseBytes ...
func ParseBytes(srcIP string, js []byte) (*Conf, error) {
	c := &Conf{}
	err := json.Unmarshal(js, c)
	if err != nil {
		return nil, err
	}
	if err = c.fix(); err != nil {
		return nil, err
	}
	_confs[srcIP] = c
	return c, nil
}

// ParseFile ...
func ParseFile(srcIP, jsfile string) (*Conf, error) {
	b, err := os.ReadFile(jsfile)
	if err != nil {
		return nil, err
	}
	return ParseBytes(srcIP, b)
}

////////////////////////////////////////////////////////////////////////////////

// fix ...
func (m *Conf) fix() error {
	if len(m.Server.Main) == 0 {
		return errors.New("missing server.main")
	}
	for _, r := range m.DNS {
		if err := r.CompileRegexp(); err != nil {
			return err
		}
	}
	for _, r := range m.Rules {
		if err := r.CompileRegexp(); err != nil {
			return err
		}
	}
	// fmt.Println(m)
	return nil
}

// GetDNSRule
func (m *Conf) GetDNSRule(target string) (rule *Rule, index int) {
	for i, r := range m.DNS {
		if r.IsMatch(target) {
			return r, i
		}
	}
	return nil, -1
}

// GetRule
func (m *Conf) GetRule(target string) (rule *Rule, index int) {
	for i, r := range m.Rules {
		if r.IsMatch(target) {
			return r, i
		}
	}
	return nil, -1
}

////////////////////////////////////////////////////////////////////////////////

// GetAuthS5 ...
func (m *Conf) GetAuthS5() (user, pass string) {
	arr := strings.Split(m.Server.Auth.S5, ":")
	if len(arr) == 2 && len(arr[0]) > 0 && len(arr[1]) > 0 {
		return arr[0], arr[1]
	}
	return
}

// GetAuthSS ...
func (m *Conf) GetAuthSS() (cipher, pass string) {
	arr := strings.Split(m.Server.Auth.SS, ":")
	if len(arr) == 2 && len(arr[0]) > 0 && len(arr[1]) > 0 {
		return arr[0], arr[1]
	}
	return
}

// GetAuthS5 ...
func GetAuthS5(srcIP string) (user, pass string) {
	if c, ok := _confs[srcIP]; ok {
		user, pass = c.GetAuthS5()
	}
	if len(user) == 0 || len(pass) == 0 {
		if c, ok := _confs[_DEFAULT_KEY]; ok {
			user, pass = c.GetAuthS5()
		}
	}
	return
}

// GetAuthSS ...
func GetAuthSS(srcIP string) (cipher, pass string) {
	if c, ok := _confs[srcIP]; ok {
		cipher, pass = c.GetAuthSS()
	}
	if len(cipher) == 0 || len(pass) == 0 {
		if c, ok := _confs[_DEFAULT_KEY]; ok {
			cipher, pass = c.GetAuthSS()
		}
	}
	return
}
