pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

function _init()
	cls()
	printh("Music:")
	dump_ram(0x3100, 256)
	printh("SFX:")
	dump_ram(0x3200, 0x1100)
end

function _update()
end

function _draw()
end

_hex_digits = "0123456789abcdef"

function tohexstr (val, digits)
	local s = ""
	for n = 1, digits do
		local v = band(val, 0xf)
		s = sub(_hex_digits, v+1, v+1)..s
		val = val/16
	end
	return s
end


function dump_ram(addr, len)
	local s = ""

	for n = 0, len-1 do
		if band(n, 0xf) == 0 then
			printh(s)
			s = ""
			s = s..tohexstr(addr+n, 4)..": "
		end
		local v = peek(addr+n)
		s = s..tohexstr(v, 2).." "
	end
	printh(s)
end

__sfx__
000800001a75021750000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000200002c060350503b0403e0403365029060216501e0501665015040116400d0400b6300a030076300803007630060300463004030036300263002640016400164001640016300163001630016300163001620
000100003c5503875034550317502f5502b55025750245501f7501f5301a720185201171010550097500655004750047500175004550047500b50008500065000350001500015000000000000000000000000000
000200002866034670396703b6603a670376603667033660306702b670286701c660075701766007560166600656014660085601366007560106600a5600d6600556009660025600566002560046600156004660
000300002f3502f3502f3502f3502f3502f3503d3503d3503c3503c3503c350353503533035330353300e30012300163001a3001e3002330026300293002b3002f300284002630035400203001c300364001b300
000600002bc602fc702fc7029c603725036f5034c6032f7038f7038f6022350223401e3301c3302ff302ff4036f4035f402fc3031f5030c7036f702fc6038f402a33038350383603636020c5026c502ec5032c60
000300000d6500e650106501265015650186501c6501e650216502165021650206501d6501b650166501665013650126500f6500e6500c6500b65008650076500665005650046500465002650016500165001650
00030000263500d300173001c30026350000001c30000000263500000000000000001b30000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000100003925038250322502c250230501d2501c0501625012050107500b0500b75003050140500c750080500a750070500775007050057500505004050020500205001050010500420004200036000420004200
000300001a6500a750266500f75032650147502c650117502965010650186500e650186500d650176500b650166500e6501a6500e650146501a6500625014650062500d650047500d65001650056500265006650
000b0000067510276106771027710677101771067710276106761027610677101571067710257106761035610676101561067610276106761027710575102751057410c741147411d75116741107410b73108721
011000001815018150181501815018155181501815018150181501815018150181550000018150181501815018150181551815018150181550000018150000001815000000181501815018150000001815018150
011000001c1501c1501c15518150181501815018150181501815018155000001815018150181501815018155181501815018155000001c150000001c15000000181501415000000181501c1501c1501c1501c150
011000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000018150181501815018150
0110000018150181501815514150141501c1501c1501c1501c1501c1550000014150141501c1501c1501c1551415014150141550000018150000001815000000141501815000000141501f1501f1501f1501f150
011000001c15518150181501c1501c1501c1501c1501c1550000014150141501c1501c1501c15500000181501815018155181501815518150000001c150000001815018150000000000000000000000000000000
01100000181551c1501c1501815018150181501815018155000001c15018150181501815018155000001c1501c1501c1551c1501c1551c1501c1551c150000001c1501c150000000000000000000000000000000
01100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001815000000181501c1501d1501d1501d1501d1501d1501d150
011000001f15514150141501f1501f1501f1501f1501f15500000181501c1501f1501f1501f155000001415014150141551f150000001f150000001f150000001f1501f150111501115011150111501115011150
011000000000000000000000000000000000000000000000000000000000000000000000000000000000000014155000000000000000000000000000000000000000000000000000000000000000000000000000
011000001d1551f1501f1501f1501f1501f1501f15520150221502015020150201502015020150201502015020155181501815018150181501815518150181551d1501d1501d1501d1501d155000001f1501f150
01100000111551315013150131501315013150131551415016150141501415014150141501415014150141500c1500c1500c1500c1500c1500c1550c1500c1551115011150111501115011155000001315013155
011000001f15520155000000c15500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0110000020150181500000020150000001d140241502215022150221502215022150221502215022150221502215022155181501815018155181501d1501d1501d1501d1501d1501d155000001f1502015020150
0110000014150141550c1501415000000111501815016150161501615016150161501615016150161501615016150161550c1500c1500c1550c15011150111501115011150111501115500000131501415014150
011000000000000000000000000024155000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
011000000000000000000000000018155000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
01100000201551d15024150241502015029150291502915029150291502915029155000001d1501d1501d1501d155201501f1501d150241502415024150241502415500000201501d14018150181501815500000
011000001415511150181501815014150000001d1501d1501d1501d1501d1501d1501d1551115011150111501115514150131501115018150181501815018150181550000014150111500c1500c1500c15500000
011000000000000000000001d1501d1501d1501d1501d1501d1501d1501d1501d1550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
011000001815018155000001815000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
011000000c1500c155000000c15011150111501115011150111501115011150111501115011150111550000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010800002e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e145000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010800002914029140291402914029140291402914029140291402914029145000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010800002614026140261402614026140261402614026140261402614026145000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010800000010000100001000010000100001000010000100001000010000100001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
01080000000000000000000000002214022140221402214022140221451d1401d14500000271402714500000291402914527140271450000022140221450000022140221451d1401d14500000271402714500000
01080000291402914527140271450000022140221450000022140221451d1401d1450000027140271450000029140291452214000000221402214022145000001d1401d1451d1401d145000001d1401d14500000
010800000514005145031400314500000051400514500000081400814008140081400814008140081400814008140081400814008140081400814008145000000514005145051400514500000051400514500000
010800002214022140221402214022140221402214022140221402214022140221402214022140221450000029140291402914029140291402914029140291402914029140291402914029140291402914500000
010800000000000000000000000000000001400014000000001400014000140001400014000140001400014000140001450000000000001400014000140000000014000140001400014500140001400014000145
0108000027140271452614026145000002414024145000002e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1452914029140291402914029140291402914500000
010800000000000000000000000003140031400314003145000000000000000000000314003140031400314503140031450314000000031400314003145000000014000145001400014500000001400014500000
0108000027140271452614026145000002414024145000002e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1402e1452914029140291402914029140291402914029145
01080000271402714526140261450000027140271450000024140241402414024140241402414024140241402414024140241402414024140241402414024145000000000000000000001d1401d1401d14000000
010800002414024145231402314500000241402414500000211402114021140211402114021140211402114021140211402114021140211402114021140211450000000000000000000000000000000000000000
010800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000051400514005140000000514005145000000000005140051450000000000
010800000a1500a1500a1500a1500a1500a1500a15500000091500915009150091500915009150091500915009150091500915009155000000000009140091450a1500a1550a1400a14500000091400914500000
0108000007150071500715007150071500715007155000000715007150071500715500000000000f1400f1450f1500f1550e1500e155000000c1500c155000000a1500a155091500915500000071500715500000
01080000051500515005150051500515005150051550000007140071450000000000091400914500000000000a1500a155091400914500000071400714500000051500515500000000000a1400a1450000000000
010800002714027145261402614500000271402714500000241402414024140241402414024140241402414024140241402414024140241402414024140241450000000000000000000000000000000000000000
010800000514005145000000000000000000000000000000051500515505140051450000005140051450000005150051500515005150051500515005150051550000000000000000000000000000000000000000
010d00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000025120000002512025120251202512525120
010d00003062500000000003062530625000000000030625000003062530625306253062530625000000000030625000003062500000000003062530625306253062521120000003062500000000003062530625
010d0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003062500000211202112021120211251e120
010d00000b1200b125000000000000000000000000000000000000b1200b1200b1200b1200b12000000000000000000000000000000000000000000b1200b1200b120281200b1202812028120281202812522120
010d00002512500000251202812025120251202512025125221202212500000281202b1202b1202b1202b1202b1252712027125281202712021120000001e1201e1201e1201e1201e1201e1201e1201e1250b120
010d00000000000000211200000030625306253062530625306250000000000241203062524120241202412030625306253062530625306252812000000251202512030625306250000000000306253062530625
010d00001e1201e1253062500000281202812028125000002512025125000003062500000000000000000000241252a1202a125241202a1203062500000000000000025120251202512025120251202512025120
010d0000221250000028120211200b1200b1200b1200b1201e1201e125000002b1202412028120281202812028125231200b1202b12023120251200000022120221202212022120221202212022120221250b120
010d0000000000b120000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010d00003062530625306250000000000000003062530625000000000030625306253062500000306253062530625000000000030625306250000000000000003062530625306253062530625000003062530625
010d0000000000b120000000b12000000000000000000000000000000000000000000b1200b1200b1200b1200b12000000000000000000000000000000000000000000b1200b1200b1200b120000000b1200b125
__music__
01 20212223
00 24404040
00 25482640
00 27402840
00 29402a40
00 2b6b2a40
00 2c2d2e40
00 27402f40
00 29403040
00 2b413140
02 322d3340
01 34353637
00 38393a3b
04 403d403e
01 0b534040
00 0c0d400e
00 0f101112
00 13401415
00 16401718
00 191a1b1c
04 1d401e1f
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000
00 00000000

