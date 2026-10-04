"""Simula la pantalla de 240x280 de la Waveshare ESP32-S3-LCD-1.69 (ejecutar desde la raiz del repositorio)."""
from PIL import Image, ImageDraw, ImageFont
import math
S=4; W,H=240,280
F="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def f(n): return ImageFont.truetype(F,n*S)
GR=(50,54,62);WH=(255,255,255);BL=(40,140,255);CY=(0,220,255);OR=(255,140,0);GN=(60,220,90);YE=(255,210,0);RD=(255,50,40);DG=(140,146,156)
RECTA=4; ALARMA=40
def screen(lean,mi,md,acc,ma,mf,gps,wifi,destello=False):
    BG=(150,0,0) if destello else (0,0,0)          # por encima de 40 grados el fondo parpadea en rojo
    GRF=(95,40,40) if destello else GR
    im=Image.new("RGB",(W*S,H*S),BG);d=ImageDraw.Draw(im)
    R=lambda x0,y0,x1,y1,c:d.rectangle([x0*S,y0*S,x1*S,y1*S],fill=c)
    T=lambda s,x,y,n,c,a="mm":d.text((x*S,y*S),s,font=f(n),fill=c,anchor=a)
    # iconos de estado: verde fijo = hay; rojo parpadeando = no hay
    def icono_gps(x,y,c):
        d.ellipse([(x-9)*S,(y-11)*S,(x+9)*S,(y+7)*S],fill=c)
        d.polygon([((x-7)*S,(y+3)*S),((x+7)*S,(y+3)*S),(x*S,(y+15)*S)],fill=c)
        d.ellipse([(x-3.5)*S,(y-5.5)*S,(x+3.5)*S,(y+1.5)*S],fill=BG)
    def icono_wifi(x,y,c):
        for rr in (17,11,5):
            d.arc([(x-rr)*S,(y-rr+8)*S,(x+rr)*S,(y+rr+8)*S],225,315,fill=c,width=int(3.4*S))
        d.ellipse([(x-2.5)*S,(y+6)*S,(x+2.5)*S,(y+11)*S],fill=c)
    icono_gps(38,30,GN if gps else RD); icono_wifi(202,28,GN if wifi else RD)
    # arco
    cx,cy,r,th=120,150,100,15
    box=[(cx-r)*S,(cy-r)*S,(cx+r)*S,(cy+r)*S]
    d.arc(box,205,335,fill=GRF,width=th*S)
    al=abs(lean);col=BL if al<25 else (YE if al<ALARMA else (WH if destello else RD))
    a0,a1=sorted([270,270+max(-65,min(65,lean))])
    if al>=RECTA: d.arc(box,a0,a1,fill=col,width=th*S)
    def tick(deg,c,l0,l1,w):
        a=math.radians(270+deg)
        d.line([((cx+(r-l0)*math.cos(a))*S,(cy+(r-l0)*math.sin(a))*S),((cx+(r-l1)*math.cos(a))*S,(cy+(r-l1)*math.sin(a))*S)],fill=c,width=w*S)
    for t in(-60,-30,30,60): tick(t,DG,-6,-2,2)
    tick(0,WH,-7,th+2,3)
    tick(-mi,CY,-4,th+2,3); tick(md,CY,-4,th+2,3)
    # numero lo mas grande posible
    T(f"{round(al)}°",cx+9,132,84,WH)
    recta=al<RECTA
    T("RECTA" if recta else ("<< IZQ" if lean<0 else "DER >>"),cx,190,23,GN if recta else col)
    # maximos
    T("MAX",cx,218,13,DG)
    T(f"{round(mi)}°",cx-12,246,36,CY,"rm"); T(f"{round(md)}°",cx+12,246,36,CY,"lm")
    # barras de frenada y aceleracion
    def bar(x,val,mx,c):
        for i in range(10):
            y=70+i*13; R(x,y,x+13,y+10,c if round(val*10)>(9-i) else GRF)
        ym=70+130-min(mx,1)*130; R(x-3,ym-2,x+16,ym,WH)
    bar(5,max(0,-acc),mf,OR); bar(222,max(0,acc),ma,GN)
    T(f"{mf:.2f}",4,212,15,OR,"lm"); T(f"{ma:.2f}",236,212,15,GN,"rm")
    m=Image.new("L",(W*S,H*S),0); ImageDraw.Draw(m).rounded_rectangle([0,0,W*S-1,H*S-1],radius=34*S,fill=255)
    out=Image.new("RGB",(W*S,H*S),(28,30,34)); out.paste(im,(0,0),m)
    return out.resize((W*2,H*2),Image.LANCZOS)
cs=[screen(2,41,38,0.0,0.45,0.72,False,False),screen(32,41,38,0.21,0.45,0.72,True,True),
    screen(-44,44,38,-0.55,0.45,0.72,True,False,destello=True)]
out=Image.new("RGB",(3*W*2+4*24,H*2+48),(28,30,34))
for i,c in enumerate(cs): out.paste(c,(24+i*(W*2+24),24))
out.save("docs/img/pantalla_169.png")
