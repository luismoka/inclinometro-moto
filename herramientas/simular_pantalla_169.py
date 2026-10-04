"""Simula la pantalla de 240x280 de la Waveshare ESP32-S3-LCD-1.69 (ejecutar desde la raiz del repositorio)."""
from PIL import Image, ImageDraw, ImageFont
import math
S=4; W,H=240,280
F="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def f(n): return ImageFont.truetype(F,n*S)
BG=(0,0,0);GR=(50,54,62);WH=(255,255,255);BL=(40,140,255);CY=(0,220,255);OR=(255,140,0);GN=(60,220,90);YE=(255,210,0);RD=(255,50,40);DG=(140,146,156)
def screen(lean,mi,md,acc,ma,mf,kmh,gps):
    im=Image.new("RGB",(W*S,H*S),BG);d=ImageDraw.Draw(im)
    R=lambda x0,y0,x1,y1,c:d.rectangle([x0*S,y0*S,x1*S,y1*S],fill=c)
    T=lambda s,x,y,n,c,a="mm":d.text((x*S,y*S),s,font=f(n),fill=c,anchor=a)
    cx,cy,r,th=120,122,88,13
    box=[(cx-r)*S,(cy-r)*S,(cx+r)*S,(cy+r)*S]
    d.arc(box,205,335,fill=GR,width=th*S)
    al=abs(lean);col=BL if al<25 else (YE if al<40 else RD)
    a0,a1=sorted([270,270+max(-65,min(65,lean))])
    if al>0.5: d.arc(box,a0,a1,fill=col,width=th*S)
    def tick(deg,c,l0,l1,w):
        a=math.radians(270+deg)
        d.line([((cx+(r-l0)*math.cos(a))*S,(cy+(r-l0)*math.sin(a))*S),((cx+(r-l1)*math.cos(a))*S,(cy+(r-l1)*math.sin(a))*S)],fill=c,width=w*S)
    for t in(-60,-30,30,60): tick(t,DG,-6,-2,2)
    tick(0,WH,-7,th+2,3)
    tick(-mi,CY,-4,th+2,3); tick(md,CY,-4,th+2,3)
    T(f"{round(al)}°",cx+6,102,60,WH)
    T("<< IZQ" if lean<-1.5 else ("DER >>" if lean>1.5 else "RECTA"),cx,146,18,col if al>1.5 else GN)
    T("MAX",cx,174,13,DG)
    T(f"{round(mi)}°",cx-14,198,27,CY,"rm"); T(f"{round(md)}°",cx+14,198,27,CY,"lm")
    def bar(x,val,mx,c):
        for i in range(10):
            y=58+i*13; R(x,y,x+15,y+10,c if round(val*10)>(9-i) else GR)
        ym=58+130-min(mx,1)*130; R(x-3,ym-2,x+18,ym,WH)
    bar(6,max(0,-acc),mf,OR); bar(219,max(0,acc),ma,GN)
    T("FRENO",6,202,11,OR,"lm"); T("ACEL",234,202,11,GN,"rm")
    T(f"{mf:.2f}",6,218,14,OR,"lm"); T(f"{ma:.2f}",234,218,14,GN,"rm")
    # linea inferior: estado del GPS (sustituye al LED) y velocidad
    R(24,232,216,233,GR)
    cg={'no':RD,'busca':YE,'ok':GN,'rec':GN,'pausa':BL}[gps]
    d.ellipse([58*S,248*S,72*S,262*S],fill=cg)
    if gps=='rec': d.ellipse([62*S,252*S,68*S,258*S],fill=RD)
    if gps in('ok','rec','pausa'): T(f"{kmh} km/h",cx+14,255,26,WH)
    else: T("sin GPS" if gps=='busca' else "GPS no responde",cx+18,255,16,DG)
    # esquinas redondeadas del cristal
    m=Image.new("L",(W*S,H*S),0); ImageDraw.Draw(m).rounded_rectangle([0,0,W*S-1,H*S-1],radius=34*S,fill=255)
    out=Image.new("RGB",(W*S,H*S),(28,30,34)); out.paste(im,(0,0),m)
    return out.resize((W*2,H*2),Image.LANCZOS)
cs=[screen(0,41,38,0.0,0.45,0.72,0,'busca'),screen(32,41,38,0.21,0.45,0.72,74,'rec'),screen(-44,44,38,-0.55,0.45,0.72,58,'pausa')]
out=Image.new("RGB",(3*W*2+4*24,H*2+48),(28,30,34))
for i,c in enumerate(cs): out.paste(c,(24+i*(W*2+24),24))
out.save("docs/img/pantalla_169.png")
