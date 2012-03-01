require("L4")
local ld = L4.default_loader

local ps2 = ld:new_channel()
local fbdrv = ld:new_channel()
local fb = ld:new_channel()
local hacky = ld:new_channel()
local fancy = ld:new_channel()

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
            log = { "io", "B" },
         }, "rom/io rom/vbus-config.vbus rom/x86-legacy.devs rom/x86-fb.io")

ld:start({
            caps = {
               vbus = fbdrv,
               fb = fb:svr(),
            },
            log = { "fb-drv", "b" },
         }, "rom/fb-drv -m 0x112")

ld:start({
	    caps = {
	       vbus = ps2,
	       hacky = hacky:svr(),
	    },
	    log = { "hacky", "R" },
	 }, "rom/hacky")

ld:start({
	    caps = {
	       fb = fb,
	       fancy = fancy:svr(),
	       hacky = hacky:create(0),
	    },
	    log = { "fancy", "Y" },
	 }, "rom/fancy DEBUG")

ld:start({
	    caps = {
	       fancy = fancy:create(0),
	       hacky = hacky:create(0),
	    },
	    log = { "spammer", "G" },
	 }, "rom/spammer DEBUG")

local colors = { "0x0000FF00", "0x000000FF" }
for i=1,2 do
   ld:start({
	       caps = {
		  fancy = fancy:create(0),
	       },
	       log = { "fancy "..i, "y" },
	    }, "rom/fancy-test "..colors[i]);
end

for i=1,2 do
   ld:start({
	       caps = {
		  hacky = hacky:create(0),
	       },
	       log = { "hacky "..i, "r" },
	    }, "rom/hacky-test");
end
