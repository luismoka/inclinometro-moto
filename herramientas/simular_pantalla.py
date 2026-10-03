from PIL import Image, ImageDraw, ImageFont
import math
S=8
F="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def f(n): return ImageFont.truetype(F,n*S)
BG=(0,0,0);GR=(50,54,62);WH=(255,255,255);BL=(40,140,255);CY=(0,220,255);OR=(255,140,0);GN=(60,220,90);YE=(255,210,0);RD=(255,50,40);DG=(140,146,156)
def screen(lean,mi,md,acc,ma,mf):
    im=Image.new("RGB",(128*S,128*S),BG);d=ImageDraw.Draw(im)
    R=lambda x0,y0,x1,y1,c:d.rectangle([x0*S,y0*S,x1*S,y1*S],fill=c)
    T=lambda s,x,y,n,c,a="mm":d.text((x*S,y*S),s,font=f(n),fill=c,anchor=a)
    cx,cy,r,th=64,66,46,7
    box=[(cx-r)*S,(cy-r)*S,(cx+r)*S,(cy+r)*S]
    d.arc(box,205,335,fill=GR,width=th*S)
    al=abs(lean);col=BL if al<25 else (YE if al<40 else RD)
    a0,a1=sorted([270,270+max(-65,min(65,lean))])
    if al>0.5: d.arc(box,a0,a1,fill=col,width=th*S)
    def tick(deg,c,l0,l1,w):
        a=math.radians(270+deg)
        d.line([((cx+(r-l0)*math.cos(a))*S,(cy+(r-l0)*math.sin(a))*S),((cx+(r-l1)*math.cos(a))*S,(cy+(r-l1)*math.sin(a))*S)],fill=c,width=w*S)
    for t in(-60,-30,30,60): tick(t,DG,-3,-1,1)
    tick(0,WH,-4,th+1,2)
    tick(-mi,CY,-2,th+1,2); tick(md,CY,-2,th+1,2)
    T(f"{round(al)}°",cx+3,56,30,WH)
    T("<< IZQ" if lean<-1.5 else ("DER >>" if lean>1.5 else "RECTA"),cx,78,9,col if al>1.5 else GN)
    T("MAX",cx,92,7,DG)
    T(f"{round(mi)}°",cx-8,103,13,CY,"rm"); T(f"{round(md)}°",cx+8,103,13,CY,"lm")
    # barras: 10 segmentos = 1.0 g
    def bar(x,val,mx,c):
        for i in range(10):
            y=20+i*7
            on=val*10>(9-i)
            R(x,y,x+9,y+5,c if on else GR)
        ym=20+70-min(mx,1)*70
        R(x-2,ym-1,x+11,ym,WH)
    bar(3,max(0,-acc),mf,OR); bar(116,max(0,acc),ma,GN)
    T("FRENO",2,13,7,OR,"lm"); T("ACEL",126,13,7,GN,"rm")
    T(f"{mf:.2f}",2,98,8,OR,"lm"); T(f"{ma:.2f}",126,98,8,GN,"rm")
    T("g max",2,107,6,DG,"lm"); T("g max",126,107,6,DG,"rm")
    T(f"{acc:+.2f} g",cx,120,9,WH)
    im=im.resize((128,128),Image.LANCZOS).resize((384,384),Image.NEAREST)
    return im
cs=[screen(0,41,38,0.0,0.45,0.72),screen(32,41,38,0.21,0.45,0.72),screen(-44,44,38,-0.55,0.45,0.72)]
out=Image.new("RGB",(3*384+4*24,384+48),(28,30,34))
for i,c in enumerate(cs): out.paste(c,(24+i*408,24))
out.save("docs/img/pantalla.png")
