require("L4")
local ld = L4.default_loader

local ps2 = ld:new_channel()
local keyboard = ld:new_channel()

local sigma0_cap =
   L4.cast(L4.Proto.Factory, L4.Env.sigma0)
   :create(L4.Proto.Sigma0)

ld:start({
            caps = {
               bus    = ps2:svr(),
               icu    = L4.Env.icu, -- TEST for exclusion
               sigma0 = sigma0_cap,
            },
            log = { "io", "w" },
         }, "rom/io rom/vbus-config.vbus rom/x86-legacy.devs")

ld:start({
	    caps = {
	       vbus = ps2,
	       keyboard = keyboard:svr(),
	    },
	    log = { "keyboard", "R" },
	 }, "rom/keyboard")

ld:start({
	    caps = {
	       keyboard = keyboard:create(0),
	    },
	    log = { "keytest1", "r" },
	 }, "rom/keytest DEBUG");

ld:start({
	    caps = {
	       keyboard = keyboard:create(0),
	    },
	    log = { "keytest2", "b" },
	 }, "rom/keytest DEBUG");