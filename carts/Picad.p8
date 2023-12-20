pico-8 cartridge // http://www.pico-8.com
version 10
__lua__
--picad 0.21
--by musurca
appname="pIcad"
version="0.21"
date="12.30.2016"

--[[
[version history]
~0.21 - 12.30.2016
-click n' drag support
-analog keyboard control
-reduce token count for template
-bugfix: brush scroll
-bugfix: ui lock
-bugfix: copy/clone w offset
(may have issues on non-even
offset)

~0.2  - 12.22.2016
-added gradient fill brush

~0.11 - 12.22.2016
-added autosave feature - work
is saved in picad_autosave.p8l
(off by default--must be enabled 
in settings)

~0.1 - 12.21.2016
-initial release

[wish list]
-polyfill mode
-x/y corner mode(??)
-more elegant ui
-ui with dropdown icons
 * combine rect+fill in one 
 column and same with circ+fill
-list view of all actions, 
w/promotion/demotion/drop in
at position
-or just a move up/down arrow
-settings:
 * canvas size (for convenience)
-use guides to demo it
-load "prefabs" as strings for
reuse or save as prefab
-*background img from string
-about screen logo
-non-even copy/clone
]]--

-- [start ui library]

null={}
function nullfunc()
end

curui=null
function ui_make()
 local screen={list_objs={}}
 return screen
end

function ui_draw()
 if curui!=null then
  for b in all(curui.list_objs) do
   if(b.visible) b.ondraw(b)
  end
 end
end

function ui_addobj(ui,a)
 ui.list_objs[#ui.list_objs+1]=a
end

function ui_removeobj(ui,a)
 del(ui.list_objs,a)
end

function ui_setactive(a)
 curui=a
end

--uiobj -- generic ui object
function uiobj_make(ui,ax,ay,aw,ah)
 --objects extending from this
 --must also define the 
 --following callbacks:
 --ondraw,onmousedown,onmouseup,
 --onmouseover,onmouseout
 
 local obj={x=ax,
            y=ay,
            w=aw,
            h=ah,
            visible=true,
            parent=ui,
            ondraw=nullfunc,
            onmouseout=nullfunc,
            onmouseover=nullfunc,
            onmousedown=nullfunc,
            onmouseup=nullfunc,
            onclick=nullfunc}
 
 ui_addobj(ui,obj)
 return obj
end

function uiobj_destroy(this)
 ui_remove(this.parent,this)
 this={}
end

function uiobj_inside(a,px,py)
 return px>=a.x and px<=(a.x+a.w) and py>=a.y and py<=(a.y+a.h)
end

--buttons -- derived from uiobj
b_up=0
b_down=1
function btn_make(ui,lbl,clickfunc,bx,by,bw,bh)
 --size dimensions to text
 bw=bw or (#lbl*4)+6
 bh=bh or 8
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,bw,bh)

 --default button state
 b.state=b_up
 b.label=lbl
 
 --define callbacks
 b.ondraw=btn_ondraw
 b.onmouseout=btn_onmouseout
 b.onmouseover=btn_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=clickfunc

 return b
end

function btn_ondraw(a)
 local fcol=6
 if(a==mouse.focus) fcol=7
 local tx=flr(0.5*(a.w-(#a.label*4)))+1
 local ty=flr(0.5*(a.h-4))
 local x0=a.x
 local x1=a.x+a.w
 local y0=a.y
 local y1=a.y+a.h
 if a.state==b_up then
  line(x0,y1,x1,y1,  1)
  line(x1,y0,x1,y1-1,5)
  rectfill(x0,y0,x1-1,y1-1,fcol)
 else
  line(x0,y0,x1,y0,5)
  line(x0,y0,x0,y1,5)
  rectfill(x0+1,y0+1,x1,y1,1)
 end
 print(a.label,x0+tx,y0+ty,0)
end

function btn_onmousedown(a)
 a.state=b_down
end

function btn_onmouseup(a)
 a.state=b_up
 a.onclick(a)
end

function btn_onmouseout(a)
 if(a.state==b_down) a.state=b_up
end

function btn_onmouseover(a)
 if(mouse.state==b_down) a.state=b_down
end

--mouse
mouse={x=0,
       y=0,
       prevx=0,
       prevy=0,
       startx=0,
       starty=0,
       state=b_up,
       visible=true,
       focus=null}

function onmousedown()
 if curui!=null then
  for b in all(curui.list_objs) do
   if b.visible and uiobj_inside(b,mouse.x,mouse.y) then
    --mouse.objover=b
    b.onmousedown(b)
    return
   end
  end
 end
  
 game_onmousedown()
end

function onmouseup()
 if curui!=null then
  for b in all(curui.list_objs) do
   if b.visible and uiobj_inside(b,mouse.x,mouse.y) then
    b.onmouseup(b)
    return
   end
  end
 end
 
 game_onmouseup()
end

mcursor_x=0
mcursor_y=0
mcursor_w=0
mcursor_h=0
mcursor_xoff=0
mcursor_yoff=0
function mouse_init(spr_index,w,h,xoff,yoff)
 xoff=xoff or 0
 yoff=yoff or 0
 
 poke(0x5f2d, 1)
 
 mcursor_x=(spr_index%16)*8
 mcursor_y=flr(spr_index/16)*8
 mcursor_w=w
 mcursor_h=h
 mcursor_xoff=xoff
 mcursor_yoff=yoff
 
 mouse.x=mid(stat(32),0,127)
 mouse.y=mid(stat(33),0,127)
 mouse.prevx=mouse.x
 mouse.prevy=mouse.y
 mouse.startx=mouse.x
 mouse.starty=mouse.y
end

mouse_analogx=0
mouse_analogy=0
function mouse_update() 
 local kx=mid(stat(32),0,127)
 local ky=mid(stat(33),0,127)
 
 if kx==mouse.startx and ky==mouse.starty then
  --check for gamepad control
  kx=0
  ky=0
  local mspd=3.5
  if(btn(1,0)) kx=1
  if(btn(0,0)) kx=kx-1
  if(btn(3,0)) ky=1
  if(btn(2,0)) ky=ky-1
  
  if check_setting(set_analogkb) then
   --simulate analog control
   local xscl,yscl=0.4,0.4
   if kx!=0 then
    xscl=1
    if sgn(kx)!=sgn(mouse_analogx) then
     mouse_analogx*=-0.5
    end
   end
   if ky!=0 then
    yscl=1
    if sgn(ky)!=sgn(mouse_analogy) then
     mouse_analogy*=-0.5
    end
   end
   mouse_analogx=mouse_analogx*xscl+kx*0.2
   mouse_analogy=mouse_analogy*yscl+ky*0.2
   kx=mouse_analogx
   ky=mouse_analogy
  else
   kx*=mspd
   ky*=mspd
  end
  
  --limit mouse speed  
  local len=sqrt(kx*kx+ky*ky)
  if len>mspd then
   kx=flr(mspd*kx/len)
   ky=flr(mspd*ky/len)
  else
   kx=flr(kx+0.5)
   ky=flr(ky+0.5)
  end
  
  mouse.prevx=mouse.x
  mouse.prevy=mouse.y
  mouse.x=mid(mouse.x+kx,0,127)
  mouse.y=mid(mouse.y+ky,0,127)
 else
  --mouse control
  mouse.prevx=mouse.x
  mouse.prevy=mouse.y
 
  mouse.startx=kx
  mouse.starty=ky
  mouse.x=kx
  mouse.y=ky
 end
 
 local mdown=(stat(34)>0) or btn(4,0)
 
 if mouse.visible then
  if mdown and mouse.state==b_up then
   --mouse depressed
   mouse.state=b_down
   onmousedown()
  elseif not(mdown) and mouse.state==b_down then
   --mouse released
   mouse.state=b_up
   onmouseup()
  end
 else
  --if mouse not visible,
  --only update position and
  --button state
  if mouse.focus!=null then
   local obj=mouse.focus
   obj.onmouseout(obj)
   mouse.focus=null
  end
 
  if mdown then 
   mouse.state=b_down
  else
   mouse.state=b_up
  end
  return
 end
 
 --if the mouse has moved
 if mouse.x!=mouse.prevx or mouse.y!=mouse.prevy then
  --mouse out callback
  if mouse.focus!=null then
   local obj=mouse.focus
   if not(uiobj_inside(obj,mouse.x,mouse.y)) then
    obj.onmouseout(obj)
    mouse.focus=null
   end
  end
 
  --mouse over callback
  if curui!=null then
   for b in all(curui.list_objs) do
    if b.visible and uiobj_inside(b,mouse.x,mouse.y) then
     mouse.focus=b
     b.onmouseover(b)
     return
    end
   end
  end
 end
end

function mouse_draw()
 if(not(mouse.visible)) return
 
 palt(14,true)
 palt(0,false)
 if(cur_frame%5==4) pal(7,cur_frame%16)
 sspr(mcursor_x,mcursor_y,mcursor_h,mcursor_w,mouse.x-mcursor_xoff,mouse.y-mcursor_yoff)
 pal()
end

-- [end ui library]

-- [custom picad ui]

function pal_onselect(a)
 current_col=a.col
 
 --reset if we set new color
 if cur_mode==dl_lineaddl then
  setmode(dl_line)
 elseif cur_mode==dl_bezieraddl then
  setmode(dl_bezier)
 end
end

function pal_ondraw(a)
 local fcol=0
 if(a==mouse.focus or current_col==a.col) fcol=6
 local x0=a.x
 local x1=a.x+a.w
 local y0=a.y
 local y1=a.y+a.h
 if a.state==b_up then
  rect(x0,y0,x1,y1,fcol) 
 else
  rect(x0,y0,x1,y1,7)
 end
 rectfill(x0+1,y0+1,x1-1,y1-1,a.col)
end

function pal_make(ui,col,bx,by)
 --size dimensions to text
 bw=7
 bh=7
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,bw,bh)

 b.col=col

 --default button state
 b.state=b_up
 
 --define callbacks
 b.ondraw=pal_ondraw
 b.onmouseout=btn_onmouseout
 b.onmouseover=btn_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=pal_onselect

 return b
end

function ico_onmouseover(a)
 btn_onmouseover(a)
 status_txt=modes[a.mode]
end

function ico_onmouseout(a)
 btn_onmouseout(a)
 status_txt=""
end

function ico_onselect(a)
 setmode(a.mode)
end

function ico_ondraw(a)
 local fcol=0
 if(a==mouse.focus) fcol=6
 local x0=a.x
 local x1=a.x+a.w
 local y0=a.y
 local y1=a.y+a.h
 if a.state==b_up then
  rect(x0,y0,x1,y1,fcol) 
 else
  rect(x0,y0,x1,y1,7)
 end
 if(cur_mode==a.mode) then
  pal(6,1)
  pal(0,13)
 end
 palt(0,false)
 palt(14,true)
 sspr(a.imgid%16*8,flr(a.imgid/16)*8,a.w-1,a.h-1,x0+1,y0+1)
 pal()
end

function ico_make(ui,sid,bx,by,newmode)
 bw=9
 bh=9
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,bw,bh)
 
 --sprite
 b.imgid=sid
 
 --mode
 b.mode=newmode
 
 --default button state
 b.state=b_up
 
 --define callbacks
 b.ondraw=ico_ondraw
 b.onmouseout=ico_onmouseout
 b.onmouseover=ico_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=ico_onselect

 return b
end

function pat_onselect(a)
 current_pat=a.pat
end

function pat_ondraw(a)
 local fcol=0
 if(a==mouse.focus or a.pat==current_pat) fcol=6
 local x0=a.x
 local x1=a.x+a.w
 local y0=a.y
 local y1=a.y+a.h
 if a.state==b_up then
  rect(x0,y0,x1,y1,fcol) 
 else
  rect(x0,y0,x1,y1,7)
 end
 if(cur_mode==a.mode) then
  pal(6,1)
  pal(0,13)
 end
 palt(0,false)
 palt(14,true)
 spr(a.imgid,x0+1,y0+1)
 pal()
end

function pat_make(ui,sid,bx,by,pat)
 bw=9
 bh=9
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,bw,bh)
 
 --sprite
 b.imgid=sid
 
 --pattern
 b.pat=pat
 
 --default button state
 b.state=b_up
 
 --define callbacks
 b.ondraw=pat_ondraw
 b.onmouseout=btn_onmouseout
 b.onmouseover=btn_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=pat_onselect

 return b
end

function bru_onselect(a)
 current_stamp=a.stamp
end

function bru_ondraw(a)
 local fcol=0
 if(a==mouse.focus or a.stamp==current_stamp) fcol=6
 local x0=a.x
 local x1=a.x+a.w
 local y0=a.y
 local y1=a.y+a.h
 if a.state==b_up then
  rect(x0,y0,x1,y1,fcol) 
 else
  rect(x0,y0,x1,y1,7)
 end
 if(cur_mode==a.mode) then
  pal(6,1)
  pal(0,13)
 end
 palt(0,false)
 palt(14,true)
 spr(16+a.stamp,x0+1,y0+1)
 pal()
end

function bru_make(ui,bx,by,bru)
 bw=9
 bh=9
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,bw,bh)
 
 --stamp
 b.stamp=bru
 
 --default button state
 b.state=b_up
 
 --define callbacks
 b.ondraw=bru_ondraw
 b.onmouseout=btn_onmouseout
 b.onmouseover=btn_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=bru_onselect

 return b
end

function img_onmouseover(a)
 btn_onmouseover(a)
 status_txt=a.label
end

function img_make(ui,lbl,clickfunc,sid,bx,by)
 bw=7
 bh=9
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,bw,bh)
 
 --sprite
 b.imgid=sid
 
 --mode
 b.label=lbl
 
 --default button state
 b.state=b_up
 
 --define callbacks
 b.ondraw=ico_ondraw
 b.onmouseout=ico_onmouseout
 b.onmouseover=img_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=clickfunc

 return b
end

function frm_ondraw(a)
 rect(a.x,a.y,a.x+a.wid,a.y+a.hei,a.frmcol)
 rectfill(a.x+1,a.y+1,a.x+a.wid-1,a.y+a.hei-1,a.bgcol)
end

function lbl_ondraw(a)
 print(a.label,a.x,a.y,a.col)
end

function frm_make(ui,bx,by,bw,bh,bgcol,frmcol)
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,0,0)

 b.bgcol=bgcol
 b.frmcol=frmcol
 b.wid=bw
 b.hei=bh
 
 --define callbacks
 b.ondraw=frm_ondraw
end

function lbl_make(ui,lbl,bx,by,col)
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,0,0)

 b.label=lbl
 b.col=col
 
 --define callbacks
 b.ondraw=lbl_ondraw

 return b
end

function sbtn_make(ui,snum,bx,by)
 local lbl=setting_to_label(check_setting(snum))
 
 --derive from uiobj
 local b=uiobj_make(ui,bx,by,16,8)

 --default button state
 b.setting=snum
 b.state=b_up
 b.label=lbl
 
 --define callbacks
 b.ondraw=btn_ondraw
 b.onmouseout=btn_onmouseout
 b.onmouseover=btn_onmouseover
 b.onmousedown=btn_onmousedown
 b.onmouseup=btn_onmouseup
 b.onclick=toggle_setting

 return b
end

-- picad vector draw functions

--picad draw ops (max 16)
dl_cls       = 0
dl_stamp     = 1
dl_flood     = 2
dl_line      = 3
dl_rect      = 4
dl_rectfill  = 5
dl_circ      = 6
dl_circfill  = 7
dl_bezier    = 8
dl_copy      = 9
dl_clone     = 10
dl_gradfill  = 11
dl_lineaddl  = 14
dl_bezieraddl= 15

--fast pattern flood
--based on code by @dw817
function picad_flood(x,y,nc,pat)
 pat=pat or 0
 local oc=pget(x,y)
	if (oc==nc) return
	local queue={}
 local add=mid(pat,0,1)
	
	function fill2(x,y)
  if pget(x,y)==oc then
   queue[#queue+add]=x*256+y
   pset(x,y,nc)
   if(x>0) fill2(x-1,y)
   if(x<127) fill2(x+1,y)
   if(y>0) fill2(x,y-1)
   if(y<127) fill2(x,y+1)
  end
	end
	fill2(x,y)
	
	if pat>0 then
	 local poff=8+8*pat
	 local q,a,b
	 for i=1,#queue do
	  q=queue[i]
	  a=flr(q/256)
	  b=q-a*256
	  if(sget(poff+a%8,b%8)==0) pset(a,b,oc)
	 end
	end
end

picad_bayer={
{0.0625,0.5625,0.1875,0.6875},
{0.8125,0.3125,0.9375,0.4375},
{0.25  ,0.75  ,0.125 ,0.625},
{1.0   ,0.5   ,0.875 ,0.375}}
gradfill_maxdist=100
function picad_gradfill(x0,y0,x1,y1,nc)
 local oc=pget(x0,y0)
 if(oc==nc) return
 local queue={}
 
 --gradient line
	local xdist,ydist=x1-x0,y1-y0
	local dist=sqrt(xdist*xdist+ydist*ydist)
	--normal
	local xnorm,ynorm=-ydist,xdist
	local nptx0,npty0=x0+xnorm,y0+ynorm
 local normc=nptx0*y0-npty0*x0
 
 local sgndir=sgn((x1*ynorm-y1*xnorm+normc)/dist)
 local d,sgnd,mod
 
 function gfill(x,y)
  d=(x*ynorm-y*xnorm+normc)/dist
  sgnd=sgn(d)
  if(sgnd==sgndir and abs(d)>dist) return
  if pget(x,y)==oc then
   pset(x,y,nc)
   if(x>0) gfill(x-1,y)
   if(x<127) gfill(x+1,y)
   if(y>0) gfill(x,y-1)
   if(y<127) gfill(x,y+1)
   if(sgnd==sgndir) queue[#queue+1]=x*256+y
  end
	end
	gfill(x0,y0)

 --apply bayer dither
 local q,a,b
	for i=1,#queue do
	 q=queue[i]
	 a=flr(q/256)
	 b=q-a*256
  
  d=abs((a*ynorm-b*xnorm+normc)/dist)
	 if(d/dist>picad_bayer[a%4+1][b%4+1]) pset(a,b,oc)
	end
end

function picad_bezier(x0,y0,x1,y1,cx,cy,col)
 local xdist,ydist=abs(x1-x0),abs(y1-y0)
 local steps=max(xdist,ydist)
 local xn,yn,t,om_t,a,b,c
 local xp,yp=x0,y0
 for i=1,steps do
  t=i/steps
  om_t=1-t
  a,b,c=om_t*om_t,2*om_t*t,t*t
  xn,yn=a*x0+b*cx+c*x1,a*y0+b*cy+c*y1
  line(xp,yp,xn,yn,col)
  xp,yp=xn,yn
 end
end

function picad_stamp(x0,y0,col,b)
 pal(7,col)
 spr(16+b,x0,y0)
 pal()
end

function picad_savebg(x,y,w,h,destaddr)
 destaddr=destaddr or 0x3e00 --mid sfx
 --make sure x,y,w,h are even
 --and within screen bounds
 local xerr,yerr=x%2,y%2
 local tx=mid(x-xerr,0,126)
 local ty=mid(y-yerr,0,126)
 local wid,hei=w+xerr,h+yerr
 wid=wid+wid%2
 hei=hei+hei%2
 if(tx+wid>128) wid=128-tx
 if(ty+hei>128) hei=128-ty
 
 local row_start,row_len=tx/2,wid/2
 local m_dest=destaddr
 local m_src=0x6000+64*ty --screen
 --copy each row
 for k=1,hei do
  memcpy(m_dest,m_src+row_start,row_len)
  m_src+=64 --next row
  m_dest+=row_len
 end

 return {addr=destaddr,x=tx,y=ty,width=wid,height=hei}
end

function picad_restorebg(ptr,x,y)
 if(ptr==nil) return
 x=x or ptr.x
 y=y or ptr.y
 
 local srcaddr=ptr.addr
 local tx,ty=x-x%2,y-y%2
 local hei,wid=ptr.height,ptr.width
 local col_offset=0
 if ty<0 then
  col_offset=-ty
  hei+=ty
  ty=0
 end
 local row_offset=0
 if tx<0 then
  row_offset=-tx/2
  tx=0
 end
 if(ty+hei>128) hei=128-ty
 local row_error=0
 if tx+wid>128 then
  row_error=(tx+wid-128)/2
 end
 
 local row_start,row_len=tx/2,wid/2
 local m_src=srcaddr+col_offset*row_len
 local m_dest=0x6000+64*ty  --screen
 for k=1,hei do
  memcpy(m_dest+row_start,m_src+row_offset,row_len-row_error-row_offset)
  m_src+=row_len
  m_dest+=64
 end
end

-- picad vector save/load
hexcode="0123456789abcdef"

function char(str,k)
 return sub(str,k,k)
end

function hexdigit(k)
 local d=k+1
 return char(hexcode,d)
end

function hexfind(chr)
 local c
 for i=1,#hexcode do
  c=char(hexcode,i)
  if c==chr then
   return i-1
  end
 end
 return -1
end

function hex_to_byte(str)
 return hexfind(char(str,1))*16+hexfind(char(str,2))
end

function byte_to_hex(k)
 local d1=flr(k/16)
 local d2=k-d1*16
 return hexdigit(d1)..hexdigit(d2)
end

--[[
picad string format:
* 0 cls: clear screen. args: one half-byte color
* 1 stamp: pattern stamp. args: two bytes x and y, one half-byte color, and one half-byte pattern index
* 2 flood: pattern flood fill. args: two bytes x and y, one half-byte color, and one half-byte pattern index
* 3 line: line. args: four bytes x0,y0,x1,y1, and one half-byte color
* 4 rect: rectangle. args: four bytes x0,y0,x1,y1, and one half-byte color
* 5 rectfill: filled rectangle. args: four bytes x0,y0,x1,y1, and one half-byte color
* 6 circ: circle. args: two bytes x and y, one byte radius, and one half-byte color
* 7 circfill: filled circle. args: two bytes x and y, one byte radius, and one half-byte color
* 8 bezier: bezier curve. args: four bytes x0,y0,x1,y1, two bytes cx,cy, and one half-byte color
* 9 copy: copies a rectangular screen fragment to memory. args: four bytes x,y,w,h (all values must be even)
* a clone: pastes a previously copied fragment to screen from memory. args: two bytes x and y (must be even)
* b gradfill: gradient flood fill. args: four bytes x0,y0,x1,y1 and one half-byte color
* c unused
* d unused
* e lineaddl: line relative to end of previous line. args: two bytes x1 and y1
* f bezieraddl: curve relative to end of previous curve. args: two bytes x1, y1 and two bytes cx, cy
]]--

p_half=1
p_byte=2
p_byte_unscl=3
dl_loadparams={{p_half}, --cls
{p_byte,p_byte,p_half,p_half}, --stamp
{p_byte,p_byte,p_half,p_half}, --flood
{p_byte,p_byte,p_byte,p_byte,p_half}, --line
{p_byte,p_byte,p_byte,p_byte,p_half}, --rect
{p_byte,p_byte,p_byte,p_byte,p_half}, --rectfill
{p_byte,p_byte,p_byte_unscl,p_half}, --circ
{p_byte,p_byte,p_byte_unscl,p_half}, --circfill
{p_byte,p_byte,p_byte,p_byte,p_byte,p_byte,p_half}, --bezier
{p_byte,p_byte,p_byte_unscl,p_byte_unscl}, --copy
{p_byte,p_byte}, --clone
{p_byte,p_byte,p_byte,p_byte,p_half}, --gradfill
{}, --unused
{}, --unused
{p_byte,p_byte}, --lineaddl
{p_byte,p_byte,p_byte,p_byte}} --bezieraddl

function picad_load(str,scale)
 scale=scale or 1
 local i=1
 local list={}

 function parse_code()
  local b=hexfind(char(str,i))
  i+=1
  return b
 end
 
 function parse_byte()
  local b=hex_to_byte(sub(str,i,i+1))
  i+=2
  return (b-63)*scale+63
 end
 
 function parse_byte_unscl()
  local b=hex_to_byte(sub(str,i,i+1))
  i+=2
  return b*scale
 end
 
 local func_lut={parse_code,
                 parse_byte,
                 parse_byte_unscl}
 
 local lp,args,code
 while i<=#str do
  code=parse_code()
  lp=dl_loadparams[code+1]
  args={code}
  for k=1,#lp do
   add(args,func_lut[lp[k]]())
  end
  add(list,args)
 end
 return list
end

function picad_save(list)
 local str,args="",""
 local dr,code,lp
 local start
 if check_setting(set_exportcls) then
  start=1
 else
  start=2
 end
 
 local func_lut={hexdigit,
                 byte_to_hex,
                 byte_to_hex}
 
 for i=start,#list do
  dr=list[i]
  code=dr[1]
  lp=dl_loadparams[code+1]
  for k=1,#lp do
   args=args..func_lut[lp[k]](dr[k+1])
  end
  str=str..hexdigit(code)..args
  args=""
 end
 
 return str
end

--todo: sanity check drawlist
--e.g. hanging dl_lineaddl,
-- or dl_clone

function picad_draw(list,x,y,starti,endi)
 starti=starti or 1
 endi=endi or #list
 endi=min(endi,#list)
 
 local offsetx,offsety=63-x,63-y
 camera(offsetx,offsety)
 
 local dl,t
 local linex=-1
 local liney=-1
 local curcol=-1
 for i=starti,endi do
  dl=list[i]
  t=dl[1]
  if t==dl_cls then
   cls(dl[2])
  elseif t==dl_stamp then
   picad_stamp(dl[2],dl[3],dl[4],dl[5])
  elseif t==dl_flood then
   picad_flood(dl[2],dl[3],dl[4],dl[5],dl[6])
  elseif t==dl_line then
   linex=dl[4]
   liney=dl[5]
   curcol=dl[6]
   line(dl[2],dl[3],linex,liney,curcol)
   line_prevx=linex
   line_prevy=liney
  elseif t==dl_lineaddl then
   if(curcol<0) curcol=current_col
   if linex<0 then
    linex=line_prevx
    liney=line_prevy
   end
   line(linex,liney,dl[2],dl[3],curcol)
   linex=dl[2]
   liney=dl[3]
   line_prevx=linex
   line_prevy=liney
  elseif t==dl_rect then
   rect(dl[2],dl[3],dl[4],dl[5],dl[6])
  elseif t==dl_rectfill then
   rectfill(dl[2],dl[3],dl[4],dl[5],dl[6])
  elseif t==dl_circ then
   circ(dl[2],dl[3],dl[4],dl[5])
  elseif t==dl_circfill then
   circfill(dl[2],dl[3],dl[4],dl[5])
  elseif t==dl_bezier then
   linex=dl[4]
   liney=dl[5]
   curcol=dl[8]
   picad_bezier(dl[2],dl[3],linex,liney,dl[6],dl[7],curcol)
   line_prevx=linex
   line_prevy=liney
  elseif t==dl_bezieraddl then
   if(curcol<0) curcol=current_col
   if linex<0 then
    linex=line_prevx
    liney=line_prevy
   end
   picad_bezier(linex,liney,dl[2],dl[3],dl[4],dl[5],curcol)
   linex=dl[2]
   liney=dl[3]
   line_prevx=linex
   line_prevy=liney
  elseif t==dl_copy then
   copy_bg=picad_savebg(dl[2]+offsetx,dl[3]+offsety,dl[4],dl[5],0x2000) --change for api
  elseif t==dl_clone then
   picad_restorebg(copy_bg,dl[2]+offsetx,dl[3]+offsety)
  elseif t==dl_gradfill then
   picad_gradfill(dl[2],dl[3],dl[4],dl[5],dl[6])
  end
 end
 camera(0,0)
 if(curcol>-1) current_col=curcol
end

----picad authoring

drawlist={}

function redraw(starti,endi)
 starti=starti or 1
 
 picad_restorebg(screen_bg)
 
 --if there's no cls, do one
 if #drawlist>0 and starti==1 then
  if drawlist[1][1]!=dl_cls then
   cls(0)
  end
 end
 picad_draw(drawlist,63,63,starti,endi)
 
 screen_bg=picad_savebg(0,0,128,128)
end

function undo()
 if drawing then
  bezier_step=false
  drawing=false
  show_ui=true
 end
 if #drawlist>1 then
  local tempcol=current_col
  local dl=drawlist[#drawlist]
  local check=drawlist[#drawlist-1][1]
  if dl[1]==dl_copy then
   copy_bg=nil
  elseif cur_mode==dl_lineaddl then
   if check!=dl_line and check!=dl_lineaddl then
    setmode(dl_line)
   end
  elseif cur_mode==dl_bezieraddl then
   if check!=dl_bezier and check!=dl_bezieraddl then
    setmode(dl_bezier)
    bezier_step=false
   end
  elseif check==dl_lineaddl then
   setmode(dl_lineaddl)
  elseif check==dl_bezieraddl then
   setmode(dl_bezieraddl)
  end
  del(drawlist,dl)
  redraw()
  if(cur_mode!=dl_lineaddl and cur_mode!=dl_bezieraddl) current_col=tempcol
 end
end

modes={}
modes[dl_cls]     ="clear screen"
modes[dl_stamp]   ="stamp"
modes[dl_flood]   ="floodfill"
modes[dl_line]    ="line"
modes[dl_rect]    ="rect"
modes[dl_rectfill]="rectfill"
modes[dl_circ]    ="circle"
modes[dl_circfill]="circlefill"
modes[dl_bezier]  ="bezier"
modes[dl_copy]    ="copy"
modes[dl_clone]   ="clone"
modes[dl_gradfill]="gradient fill"

function setmode(mode) 
 if mode==dl_flood then
  show_patterns()
 elseif cur_mode==dl_flood and cur_mode!=mode then
  hide_patterns()
 end
 if mode==dl_stamp then
  show_stamps()
 elseif cur_mode==dl_stamp and cur_mode!=mode then
  hide_stamps()
 end
 
 cur_mode=mode
 bezier_step=false
 drawing=false
 ui_viz=true
 show_ui=true
end

function about()
 ui_modalbox=true
 abt_len.label="size: "..#picad_save(drawlist).." bytes"
 ui_setactive(aboutui)
end

function imgsave()
 ui_modalbox=true
 ui_setactive(saveui)
end

function savetosheet()
 picad_restorebg(screen_bg)
 cstore(0,0x6000,128*128/2,"picad_export.p8")
 ui_modalbox=false
end

function savetostring()
 local str=picad_save(drawlist)
 printh(str,"picad_string",true)
 ui_modalbox=false 
end

function autosave()
 local str=picad_save(drawlist)
 printh(str,"picad_autosave",true)
end

function imgload()
  ui_modalbox=true
  ui_setactive(loadui)
end

function loadclip()
 local newlist
	newlist=picad_load(stat(4))
 if newlist!=nil then
  drawlist=newlist
 end
 ui_modalbox=false
 redraw()
end

function closemodal()
 ui_modalbox=false
end

function opensettings()
  ui_modalbox=true
  ui_setactive(settingsui)
end

bru_btns={}

function show_stamps()
 for i=1,#bru_btns do
  bru_btns[i].visible=true
 end
end

function hide_stamps()
 for i=1,#bru_btns do
  bru_btns[i].visible=false
 end
end

pat_btns={}

function show_patterns()
 for i=1,#pat_btns do
  pat_btns[i].visible=true
 end
end

function hide_patterns()
 for i=1,#pat_btns do
  pat_btns[i].visible=false
 end
end

set_exportcls   =0
set_continueline=1
set_autosave    =2
set_clickndrag  =3
set_analogkb    =4

s_false=1
s_true =2

set_default={}
set_default[set_exportcls]=s_true
set_default[set_continueline]=s_false
set_default[set_autosave]=s_false
set_default[set_clickndrag]=s_false
set_default[set_analogkb]=s_true

function check_setting(snum)
 if(dget(snum)==0) dset(snum,set_default[snum]) 
 if(dget(snum)==s_false) return false 
 return true
end

function toggle_setting(a)
 local s=dget(a.setting)
 s=((s-1)+1)%2+1
 dset(a.setting,s)
 a.label=setting_to_label(check_setting(a.setting))
end

function setting_to_label(val)
 if(val) return "on"
 return "off"
end

function brush_tostack()
 local x0=drawargs[1]
 local y0=drawargs[2]
 local x1=drawargs[3]
 local y1=drawargs[4]

 if cur_mode==dl_cls then
  drawlist={}
  add(drawlist,{dl_cls,current_col})
 elseif cur_mode==dl_stamp then
  add(drawlist,{dl_stamp,x0,y0,current_col,current_stamp})
 elseif cur_mode==dl_flood then
  local bgcol=pget(x0,y0)
  if current_col!=bgcol then
   add(drawlist,{dl_flood,x0,y0,current_col,current_pat})
  end
 elseif cur_mode==dl_rect or cur_mode==dl_rectfill then
  add(drawlist,{cur_mode,x0,y0,x1,y1,current_col})
 elseif cur_mode==dl_line then
  line_prevx=x1
  line_prevy=y1
  add(drawlist,{dl_line,x0,y0,x1,y1,current_col})
 elseif cur_mode==dl_lineaddl then
  add(drawlist,{dl_lineaddl,x0,y0})
 elseif cur_mode==dl_circ or cur_mode==dl_circfill then
  local rx=x1-x0
  local ry=y1-y0
  local rad=sqrt(rx*rx+ry*ry)  
  add(drawlist,{cur_mode,x0,y0,rad,current_col})
 elseif cur_mode==dl_bezier then
  line_prevx=x1
  line_prevy=y1
  add(drawlist,{dl_bezier,x0,y0,x1,y1,bezier_cx,bezier_cy,current_col})
 elseif cur_mode==dl_bezieraddl then
  add(drawlist,{dl_bezieraddl,x0,y0,bezier_cx,bezier_cy})
 elseif cur_mode==dl_copy then
  local cx=min(x0,x1)
  local cy=min(y0,y1)
  local fx=max(x0,x1)
  local fy=max(y0,y1)
  --replace a previous copy
  local copycmd={dl_copy,cx,cy,fx-cx,fy-cy}
  local lastdraw=drawlist[#drawlist]
  if lastdraw[1]==dl_copy then
   drawlist[#drawlist]=copycmd
  else
   add(drawlist,copycmd)
  end
 elseif cur_mode==dl_clone then
  if copy_bg!=nil then
   local px=x0-copy_bg.width/2+1
   local py=y0-copy_bg.height/2+1
   add(drawlist,{dl_clone,px+px%2,py+py%2})
  end
 elseif cur_mode==dl_gradfill then
  local xd=x1-x0
  local yd=y1-y0
  local gd=sqrt(xd*xd+yd*yd)
  if gd>gradfill_maxdist then
   xd=flr(gradfill_maxdist*xd/gd)
   yd=flr(gradfill_maxdist*yd/gd)
  end
  add(drawlist,{dl_gradfill,x0,y0,x0+xd,y0+yd,current_col})
 end
 redraw(#drawlist)

 if cur_mode==dl_lineaddl or cur_mode==dl_bezieraddl then
  line_prevx=x0
  line_prevy=y0
 end
 
 if(check_setting(set_autosave)) autosave()
end

function brush_draw()
 local x0=drawargs[1]
 local y0=drawargs[2]
 local x1=drawargs[3]
 local y1=drawargs[4]
   
 if cur_mode==dl_line then
  line(x0,y0,x1,y1,current_col)
 elseif cur_mode==dl_gradfill then
  local xd=x1-x0
  local yd=y1-y0
  local gd=sqrt(xd*xd+yd*yd)
  if gd>gradfill_maxdist then
   xd=flr(gradfill_maxdist*xd/gd)
   yd=flr(gradfill_maxdist*yd/gd)
  end
  line(x0,y0,x0+xd,y0+yd,current_col)
 elseif cur_mode==dl_bezier then
  if bezier_step then
   picad_bezier(x0,y0,x1,y1,bezier_cx,bezier_cy,current_col)
  else
   line(x0,y0,x1,y1,current_col)
  end
 elseif cur_mode==dl_bezieraddl then
  picad_bezier(line_prevx,line_prevy,x0,y0,bezier_cx,bezier_cy,current_col)
 elseif cur_mode==dl_rect then
  rect(x0,y0,x1,y1,current_col)
 elseif cur_mode==dl_rectfill then
  rectfill(x0,y0,x1,y1,current_col)
 elseif cur_mode==dl_circ then
  local rx=x1-x0
  local ry=y1-y0
  local rad=sqrt(rx*rx+ry*ry)
  circ(x0,y0,rad,current_col)
 elseif cur_mode==dl_circfill then
  local rx=x1-x0
  local ry=y1-y0
  local rad=sqrt(rx*rx+ry*ry)
  circfill(x0,y0,rad,current_col)
 elseif cur_mode==dl_copy then
  if cur_frame%2>0 then
   rect(x0,y0,x1,y1,cur_frame%16)
  end
 end
end

function brush_start()
 drawargs[1]=mouse.x
 drawargs[2]=mouse.y
 drawargs[3]=mouse.x
 drawargs[4]=mouse.y
 
 if cur_mode==dl_cls or cur_mode==dl_stamp or cur_mode==dl_flood or cur_mode==dl_lineaddl then
   brush_tostack()
   return
 elseif cur_mode==dl_bezieraddl then
  bezier_step=true
 elseif cur_mode==dl_copy or cur_mode==dl_clone then
  local xerr=mouse.x%2
  local yerr=mouse.y%2
  drawargs[1]=mouse.x-xerr
  drawargs[2]=mouse.y-yerr
   
  if cur_mode==dl_clone then
   brush_tostack()
   return
  end
   
  drawargs[3]=mouse.x-xerr
  drawargs[4]=mouse.y-yerr
 end
  
 drawing=true
 ui_viz=show_ui
 show_ui=false
end

function brush_stop()
 if cur_mode==dl_bezier then
  if bezier_step==false then
   bezier_step=true
   return
  else
   bezier_step=false
  end
 end
  
 brush_tostack()
  
 --switch to clone
 if(cur_mode==dl_copy) setmode(dl_clone)
 
 if check_setting(set_continueline) then
  if cur_mode==dl_line then
   setmode(dl_lineaddl)
  elseif cur_mode==dl_bezier then
   setmode(dl_bezieraddl)
  end
 end

 drawing=false
 show_ui=ui_viz  
end

function game_onmousedown()
 if(ui_modalbox) return
 if not drawing and check_setting(set_clickndrag) then
  brush_start()
 end
end

function game_onmouseup()
 if(ui_modalbox) return

 if not drawing then
  if not check_setting(set_clickndrag) then 
   brush_start()
  end
 else
  brush_stop()
 end
end

current_col  =7
current_pat  =0
current_stamp=0

cur_frame=0
cur_mode=dl_cls
show_ui=true
show_guides=false
status_txt=""
ui_modalbox=false
ui_viz=true
drawing=false
drawargs={}

bezier_step=false
bezier_cx=0
bezier_cy=0

function _init()
 cartdata("__picad_vector__")
 
 menuitem(1,"about...",about)
 menuitem(2,"save...",imgsave)
 menuitem(3,"load...",imgload)
 menuitem(4,"settings...",opensettings)
 menuitem(5,"----------",nullfunc)
 
 --enable the mouse
	mouse_init(0,7,7,3,3)
	
	mainui=ui_make()
 ui_setactive(mainui)
 
 --set up main ui
 --color palette
 for i=1,16 do
  add(btn_pal,pal_make(mainui,i-1,(i-1)*8,120))
 end
 --brush icons
 for i=0,dl_gradfill do
  ico_make(mainui,32+i,10*i,0,i)
 end
 img_make(mainui,"undo",undo,44,120,0)
 --patterns
 local pb
 for i=0,14 do
  pb=pat_make(mainui,1+i,10*i,110,i)
  pb.visible=false
  add(pat_btns,pb)
 end
 --stamps
 for i=0,4 do
  pb=bru_make(mainui,10+10*i,110,i)
  pb.visible=false
  add(bru_btns,pb)
 end
 for i=5,11 do
  pb=bru_make(mainui,10+10*i,110,i)
  pb.visible=false
  add(bru_btns,pb)
 end

 --load ui
 loadui=ui_make()
 frm_make(loadui,20,30,88,42,0,7)
 frm_make(loadui,22,48,84,10,1,6)
 lbl_make(loadui,"paste string into\npico-8 window:",23,33,7)
 loadlbl=lbl_make(loadui,"",25,51,12)
 btn_make(loadui,"load!",loadclip,30,62)
 btn_make(loadui,"cancel",closemodal,68,62)

 --save ui
 saveui=ui_make()
 frm_make(saveui,20,30,88,62,0,7)
 btn_make(saveui,"save to spritesheet",savetosheet,23,33)
 lbl_make(saveui,"-> picad_export.p8",27,43,7)
 btn_make(saveui,"  save to string   ",savetostring,23,58)
 lbl_make(saveui,"-> picad_string.p8l",27,68,7)
 btn_make(saveui,"cancel",closemodal,49,80)
 
 --about ui
 aboutui=ui_make()
 frm_make(aboutui,15,20,98,92,0,7)
 local abthead=appname.." ("..version..")"
 lbl_make(aboutui,abthead,63-(#abthead*2)+1,23,7)
 lbl_make(aboutui,"by @musurca "..date.."\n„„„„„„„„„„„\n\n\n\n\n\n\n„„„„„„„„„„„",21,30,7)
 lbl_make(aboutui,"1—: show/hide ui\n2Ž: undo\n2—: show/hide guides\n2‹‘: scroll color\n2”ƒ: scroll brush",23,45,7)
 abt_len=lbl_make(aboutui,"length: ",27,90,12)
 btn_make(aboutui,"okay!",closemodal,52,100)
 
 --settings ui
 settingsui=ui_make()
 frm_make(settingsui,20,20,88,67,0,7)
 lbl_make(settingsui,"export cls cmds:",23,25,7)
 sbtn_make(settingsui,set_exportcls,90,23)
 lbl_make(settingsui,"continue lines:",23,35,7)
 sbtn_make(settingsui,set_continueline,90,33)
 lbl_make(settingsui,"autosave:",23,45,7)
 sbtn_make(settingsui,set_autosave,90,43)
 lbl_make(settingsui,"click & drag:",23,55,7)
 sbtn_make(settingsui,set_clickndrag,90,53)
 lbl_make(settingsui,"analog keyboard:",23,65,7)
 sbtn_make(settingsui,set_analogkb,90,63)
 btn_make(settingsui,"save",closemodal,52,77)
 
 add(drawlist,{dl_cls,13})

 redraw()
end

function handle_keys()
 if(btnp(5)) show_ui=not show_ui
 if btnp(0,1) then
  current_col-=1
  if(current_col<0) current_col=15
 end
 if(btnp(1,1)) current_col=(current_col+1)%16
 if btnp(2,1) then
  local m=cur_mode-1
  if(m<0) m=dl_gradfill
  setmode(m)
 end
 if(btnp(3,1)) setmode((cur_mode+1)%(dl_gradfill+1))
 if(btnp(4,1)) undo()
 if(btnp(5,1)) show_guides=not show_guides
end

function _update60()
 if not ui_modalbox then
  handle_keys()
  if show_ui then
   ui_setactive(mainui)
  else
   ui_setactive(null)
  end
 else
  show_ui=true
  drawing=false
  if curui==loadui then
   local clipb=stat(4)..""
   local lstr=""
   if #clipb>20 then
    lstr=lstr..sub(clipb,1,17).."..."
   else
    lstr=clipb
   end
   loadlbl.label=lstr
  end
 end
 
 mouse_update()
 
 --update draw args
 if drawing then
  if cur_mode==dl_copy or cur_mode==dl_clone then
   drawargs[3]=mouse.x+mouse.x%2
   drawargs[4]=mouse.y+mouse.y%2
  elseif (cur_mode==dl_bezier or cur_mode==dl_bezieraddl) and bezier_step==true then
   bezier_cx=mouse.x
   bezier_cy=mouse.y
  else
   drawargs[3]=mouse.x
   drawargs[4]=mouse.y
  end
 end
 
 cur_frame+=1
end

function _draw()
 --restore image
 picad_restorebg(screen_bg)

 --guides
 if show_guides then
  local dt=cur_frame%2
  local gcol=dt*1+(1-dt)*2
  for i=1,7 do
   local coord=i*16-1
   line(coord,0,coord,127,gcol)
   line(0,coord,127,coord,gcol)
  end
 end
 
 -- draw current brush
 if drawing then
  brush_draw()
 elseif cur_mode==dl_clone and not ui_modalbox then
  if mouse.focus==null then
   if copy_bg!=nil then
    local px=mouse.x-copy_bg.width/2+1
    local py=mouse.y-copy_bg.height/2+1
    picad_restorebg(copy_bg,px+px%2,py+py%2)
   end
  end
 elseif cur_mode==dl_stamp then
  picad_stamp(mouse.x,mouse.y,current_col,current_stamp)
 elseif cur_mode==dl_lineaddl or cur_mode==dl_bezieraddl then
  line(line_prevx,line_prevy,mouse.x,mouse.y,current_col)
 end
 
 --draw ui
 if show_ui then
  ui_draw()
  
  if not ui_modalbox then
   local colcycle=cur_frame%2*13
   print(status_txt,1,12,colcycle)
   print((mouse.x-63)..","..(mouse.y-63),
        100,12,colcycle)
  end
 end
 
 --draw mouse
 mouse_draw()
end
__gfx__
eee7eee0777777777070707070007000707070707000700070007000070000000000000000070000777700000007000000770077700077070000000000000000
eee7eee0777777770707070707070707000000000070007000000000000007000000000000777000777700000007000007700770000077000000000000000000
eeeeeee0777777777070707000700070707070707000700000700070007000000000000007777700777700000007000077007700770000000000000000000000
77eee770777777770707070707070707000000000070007000000000000000700000700077777770777700007777777770077007770770000000000000000000
eeeeeee0777777777070707070007000707070707000700070007000000070000000000007777700000077770000000700770077000770770000000000000000
eee7eee0777777770707070707070707000000000070007000000000700000000000000000777000000077770000000707700770000000770000000000000000
eee7eee0777777777070707000700070707070707000700000700070000007000000000000070000000077770000000777007700007700000000000000000000
00000000777777770707070707070707000000000070007000000000000700077000000000000000000077777777777770077007707700070000000000000000
77000000077000000777700070000000077077707070707070007000707070707000700070007000070000000000000000000000000000000000000000000000
77700000777700007777770070000000007777000707070707070707000000000070007000000000000007000000000000000000000000000000000000000000
07700000777770007777770077000000770777707070707000700070707070707000700000700070007000000000000000000000000000000000000000000000
00000000777777007777770007000000077770700707070707070707000000000070007000000000000000700000700000000000000000000000000000000000
00000000077777007777770007000000007777777070707070007000707070707000700070007000000070000000000000000000000000000000000000000000
00000000007700000777700000000000077077000707070707070707000000000070007000000000700000000000000000000000000000000000000000000000
00000000000000000000000000000000000070007070707000700070707070707000700000700070000007000000000000000000000000000000000000000000
00000000000000000000000000000000000000000707070707070707000000000070007000000000000700077000000000000000000000000000000000000000
66666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666066600000000000000000000000000
66000006660006666666666666666666600000066000000666600666666006666006666660606066660000666005567660666600000000000000000000000000
66000006600000666666006666066666606666066055550666066066660550666660006666666666666006666005567600000600000000000000000000000000
66606666600000666666660666606666606666066055550660666606605555066666606660666066666006666005567660666000000000000000000000000000
60600606660000666000660066660666606666066055550660666606605555066666606666666666666666666005567666066000000000000000000000000000
66660660660006666060000666666066600000066000000666066066660550666666660660606066600000066005567666666000000000000000000000000000
60660606666666666666006666666606666666666666666666600666666006666666660666666666660660666005567666660600000000000000000000000000
66066666666666660666666666666666666666666666666666666666666666666666666666666666666666666666666666606600000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

__gff__
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__map__
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__sfx__
000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__music__
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344

