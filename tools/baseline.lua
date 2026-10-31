-- Native cabinet operations only. No RAM writes or skipped boot checks.
local root=assert(os.getenv('RCHASE2_RESEARCH_DIR'))
local mode=os.getenv('RCHASE2_DIAGNOSTIC_MODE') or 'attract'
local machine=manager.machine
local ports=machine.ioport.ports
local log=assert(io.open(root..'/events.txt','w'))
for tag,port in pairs(ports) do
    for name,field in pairs(port.fields) do log:write(string.format('PORT %s | %s | %x\n',tag,name,field.mask)) end
end
local function set(tag,mask,value)
    for _,field in pairs(assert(ports[tag]).fields) do if field.mask==mask then field:set_value(value);return end end
    error('Missing field '..tag..' '..mask)
end
local function press(tag,mask,enabled) set(tag,mask,enabled and 1 or 0) end
local previous=-1
rchase2_diagnostic_subscription=emu.add_machine_frame_notifier(function()
    local t=machine.time:as_double();local second=math.floor(t)
    if second~=previous then
        previous=second
        log:write(string.format('TIME %.6f IP %08x IN0 %02x IN1 %02x X1 %02x Y1 %02x X2 %02x Y2 %02x\n',t,machine.devices[':maincpu'].state['ip'].value,ports[':IN0']:read(),ports[':IN1']:read(),ports[':P1_X']:read(),ports[':P1_Y']:read(),ports[':P2_X']:read(),ports[':P2_Y']:read()));log:flush()
        if second==1 or second%5==0 then
            for _,screen in pairs(machine.screens) do screen:snapshot(root..string.format('/frame-%03d.png',second));break end
        end
        if second>0 then
            local out=assert(io.open(root..string.format('/workram-%03d.bin',second),'wb'))
            out:write(machine.devices[':maincpu'].spaces.program:read_range(0x500000,0x5fffff,8));out:close()
            local sram=assert(io.open(root..string.format('/sram-%03d.bin',second),'wb'))
            sram:write(machine.devices[':maincpu'].spaces.program:read_range(0x1d00000,0x1d03fff,8));sram:close()
        end
    end
    if mode=='gameplay' then
        press(':IN0',0x01,(t>=18 and t<18.2) or (t>=19 and t<19.2))
        press(':IN0',0x10,t>=20 and t<20.3)
        press(':IN0',0x02,(t>=32 and t<32.2) or (t>=33 and t<33.2))
        press(':IN0',0x20,t>=34 and t<34.3)
        set(':P1_X',0xff,0x82);set(':P1_Y',0xff,0x73)
        set(':P2_X',0xff,0x7d);set(':P2_Y',0xff,0x73)
        press(':IN1',1,t>=24 and t<42)
        press(':IN1',2,t>=32 and t<48)
    elseif mode=='service' then
        press(':IN0',0x04,t>=10 and t<10.2)
    end
end)
