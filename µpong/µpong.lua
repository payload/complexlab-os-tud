require("L4")
local ld = L4.default_loader

local ps2 = ld:new_channel()
local keyboard = ld:new_channel()
local fbdrv = ld:new_channel()
local fb = ld:new_channel()

local sigma0_cap =
   L4.cast(L4.Proto.Factory, L4.Env.sigma0)
   :create(L4.Proto.Sigma0)

ld:start({
            caps = {
               bus    = ps2:svr(),
	       fbdrv  = fbdrv:svr(),
               icu    = L4.Env.icu, -- TEST for exclusion
               sigma0 = sigma0_cap,
            },
            log = { "io", "W" },
         }, "rom/io rom/vbus-config.vbus rom/x86-legacy.devs rom/x86-fb.io")

ld:start({
            caps = {
               vbus = fbdrv,
               fb = fb:svr(),
            },
            log = { "fb-drv", "O" },
         }, "rom/fb-drv -m 0x112")

ld:start({
	    caps = {
	       fb = fb,
	    },
	    log = { "fancy", "Y" },
	 }, "rom/fancy")

ld:start({
	    caps = {
	       vbus = ps2,
	       keyboard = keyboard:svr(),
	    },
	    log = { "hacky", "R" },
	 }, "rom/hacky")

ld:start({
	    caps = {
	       keyboard = keyboard:create(0),
	    },
	    log = { "hackyC1", "r" },
	 }, "rom/hacky-test DEBUG");

ld:start({
	    caps = {
	       keyboard = keyboard:create(0),
	    },
	    log = { "hackyC2", "b" },
	 }, "rom/hacky-test DEBUG");
