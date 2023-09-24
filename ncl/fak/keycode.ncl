let util = import "util_functions.ncl" in

let mod_ = fun K t => let K = fun mod => K t { mods."%{mod}" = true } in {
  lctl = K "left_ctrl",
  lsft = K "left_shift",
  lalt = K "left_alt",
  lgui = K "left_gui",
  rctl = K "right_ctrl",
  rsft = K "right_shift",
  ralt = K "right_alt",
  rgui = K "right_gui",
} in

let hid_ = [
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
  "ENT", "ESC", "DEL", "TAB", "SPC", "-", "=", "{", "}", "\\", "#",
  ";", "'", "`", ",", ".", "/", "CAPS", 
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
  "PSCR", "SCRL", "PAUS", "INS", "HOME", "PGUP", "DEL", "END", "PGDN",
  "RGHT", "LEFT", "DOWN", "UP", "NUM", "PSLS", "PAST", "PMNS", "PPLS", "PENT",
  "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9", "P0",
  "PDOT", "NUBS", "APP", "PWR", "PEQL",
  "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
] in

{
  tap = let 
    K = fun t d => { type = 'hold_tap, data.tap = { type = t, data = d } }
  in {
    none = K 'none {},
    reg = {
      kc = let _K = fun x => K 'regular { hid_code = x } in (
        std.array.reduce_left (&) (
          std.array.map
            (fun { index, value } => { "%{value}" = _K (index + 4) })
            (util.array.enumerate hid_)
        )
      ),
      mod = mod_ K 'regular
    },
    layer = let K = fun i op => K 'layer { layer = i, operation = op } in {
      DF = fun i => K i 'df,
      TG = fun i => K i 'tog,
      TO = fun i => K i 'to,
    },
    custom = let K = fun x => K 'custom { custom_code = x } in {
      BOOT = K 1,
    },
    trans = K 'transparent {},
  },
  
  hold = let
    K = fun t d => { type = 'hold_tap, data.hold = { type = t, data = d } }
  in {
    none = K 'none {},
    reg = {
      behavior = fun b => K 'regular { behavior = b },
      layer = fun i => K 'regular { layer = i },
      mod = mod_ K 'regular
    },
    nop = fun b => K 'nop { behavior = b },
    trans = K 'transparent {}
  },
  
  td = {
    make = fun t b => {
      type = 'tap_dance,
      data = {
        tapping_term_ms = t,
        bindings = b
      }
    }
  },
  
  MO = fun i => hold.reg.layer i,
  LT = fun b i tap => hold.reg.behavior b & hold.reg.layer i & tap,
  LM = fun b i mods tap => LT b i tap,
  DF = fun i => tap.layer.DF i,
  TG = fun i => tap.layer.TG i,
  TO = fun i => tap.layer.TO i,
  CTL_T = fun b tap => hold.reg.behavior b & hold.reg.mod.lctl & tap,
}