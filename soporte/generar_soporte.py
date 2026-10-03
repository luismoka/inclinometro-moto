import numpy as np, trimesh
from trimesh.creation import box, cylinder
from trimesh.transformations import rotation_matrix as rot, translation_matrix as tr
from PIL import Image, ImageDraw, ImageFont
def B(x0,x1,y0,y1,z0,z1):
    m=box(extents=[x1-x0,y1-y0,z1-z0]); m.apply_translation([(x0+x1)/2,(y0+y1)/2,(z0+z1)/2]); return m
def cylX(r,x0,x1,y=0,z=0,s=96):
    m=cylinder(radius=r,height=x1-x0,sections=s); m.apply_transform(rot(np.pi/2,[0,1,0])); m.apply_translation([(x0+x1)/2,y,z]); return m
def cylZ(r,z0,z1,x=0,y=0,s=32):
    m=cylinder(radius=r,height=z1-z0,sections=s); m.apply_translation([x,y,(z0+z1)/2]); return m
U=lambda l: trimesh.boolean.union(l,engine='manifold')
D=lambda a,l: trimesh.boolean.difference([a]+l,engine='manifold')
# --- parametros (mm)
RB=14.3          # radio manillar 28.6
GOMA=0.7         # holgura para goma
PX,PY,PT=34.5,25.43,6.5   # medido con calibre   # placa: largo, ancho, grosor total estimado
HUECO_PLACA=7.45   # fondo del hueco: USB 3,35 + placa 1,7 + cristal 1,85 + 0,5 de holgura
W,H,Dp=44.0,32.0,HUECO_PLACA+2.0    # caja exterior
TAPA=2.4
ANG=np.radians(45); CY,CZ=-3.0,37.0
T=tr([0,CY,CZ])@rot(ANG,[1,0,0])
def L(m): m=m.copy(); m.apply_transform(T); return m
# --- cuerpo
silla=B(-17,17,-14,14,3,21)
losa=B(-17,17,-14,14,19,21)
caja=L(B(-W/2,W/2,-H/2,H/2,-Dp/2,Dp/2))
cuello=trimesh.convex.convex_hull(np.vstack([losa.vertices,caja.vertices]))
cav=U([L(B(-17.6,17.6,-13.1,13.1,-Dp/2+2,Dp/2+1)),L(B(-16.5,-9,12,14.4,-Dp/2+2,Dp/2+1))])
SUELO=-Dp/2+2
usb=L(B(-W/2-1,-15,-5.5,7.5,SUELO-1.0,SUELO+5.5))
# apoyos en el extremo de la pantalla, a la altura del conector USB, para que la placa quede recta
apoyos=[L(B(14.4,17.7,sv*10.2 if sv>0 else -13.2,13.2 if sv>0 else -10.2,SUELO-0.2,SUELO+3.4)) for sv in (1,-1)]
tornillos=[L(cylZ(0.9,-2,Dp/2+1,x=sx*19.8,y=sy*11.5)) for sx in(-1,1) for sy in(-1,1)]
barra=cylX(RB+GOMA,-40,40)
tun=[B(x-2.9,x+2.9,-30,30,16.3,18.9) for x in(-11,11)]
cuerpo=U([D(U([cuello,silla]),[barra,cav,usb]+tun+tornillos)]+apoyos)
# --- tapa con ventana
tapa0=B(-W/2,W/2,-H/2,H/2,Dp/2,Dp/2+TAPA)
# ventana con los bordes en bisel hacia fuera, para ver la pantalla de lado
vent=trimesh.convex.convex_hull(np.array([[u,v,w] for w,e in ((Dp/2-0.5,0.0),(Dp/2+TAPA+0.5,2.0)) for u in (-9.1-e,8.6+e) for v in (-8.55-e,7.95+e)]))
# rebaje interior sobre los botones: deja sitio al ala de los pulsadores con la tapa pegada al cristal
REBAJE=0.73
rebaje=B(-17.6,-10.9,-11,11,Dp/2-0.5,Dp/2+REBAJE)
ag=[cylZ(1.2,Dp/2-1,Dp/2+TAPA+1,x=sx*19.8,y=sy*11.5) for sx in(-1,1) for sy in(-1,1)]
bot=[cylZ(2.5,Dp/2-1,Dp/2+TAPA+1,x=-14.35,y=by) for by in(7.1,0.0,-7.1)]
tapaL=D(tapa0,[vent,rebaje]+ag+bot)
tapa=L(tapaL)
cuerpo.export('soporte/soporte_cuerpo.stl')
# tapa tumbada para imprimir
# la tapa se exporta con la cara exterior hacia la cama: el rebaje interior queda arriba y sale limpio
tp=tapaL.copy(); tp.apply_transform(rot(np.pi,[1,0,0])); tp.apply_translation([0,0,-tp.bounds[0][2]]); tp.export('soporte/soporte_tapa.stl')
# --- pulsadores (seta): ala por dentro de la tapa, vastago que asoma 3 mm
# Alturas sobre la placa blanca (calibre): punta del boton 2,03, cuerpo del boton 1,55, cristal 1,85
ALA_H=0.8; ALA_D=6.4; VAST_D=4.6; SOBRESALE=3.0; JUEGO=0.25
VAST_H=(TAPA-REBAJE)+SOBRESALE+JUEGO
puls=U([cylZ(ALA_D/2,0,ALA_H,s=64),cylZ(VAST_D/2,ALA_H-0.01,ALA_H+VAST_H,s=64)])
puls.export('soporte/soporte_pulsador.stl')
tres=U([puls.copy().apply_translation([i*9.0,0,0]) for i in range(3)])
tres.export('soporte/soporte_pulsadores_x3.stl')
print('estancos:',cuerpo.is_watertight,tapa.is_watertight,'| cuerpo',cuerpo.extents.round(1),'tapa',tp.extents.round(1))
# --- piezas de ambiente para el dibujo
pcb=L(B(-PX/2,PX/2,-PY/2,PY/2,SUELO+3.4,SUELO+5.1))
lcd=L(B(-9.06,12.3,-9.12,8.51,SUELO+5.1,SUELO+6.95))
man=cylX(RB,-60,60)
# --- rasterizador simple con z-buffer
def render(objs,az,el,size=(620,560),scale=7.0,center=(0,0,20)):
    SS=2; Wd,Hd=size[0]*SS,size[1]*SS
    a,e=np.radians(az),np.radians(el)
    f=np.array([np.cos(e)*np.sin(a),-np.cos(e)*np.cos(a),np.sin(e)])   # hacia la camara
    r=np.cross([0,0,1],f); r/=np.linalg.norm(r); u=np.cross(f,r)
    img=np.zeros((Hd,Wd,3))+np.array([28,30,34]); zb=np.full((Hd,Wd),-1e9)
    lightdir=np.array([-0.35,-0.5,0.8]); lightdir/=np.linalg.norm(lightdir)
    for m,col in objs:
        V=m.vertices-np.array(center); P=np.c_[V@r*scale*SS+Wd/2, Hd/2-V@u*scale*SS, V@f]
        N=m.face_normals; sh=0.35+0.65*np.clip(N@lightdir,0,1)
        for fi,tri in enumerate(m.faces):
            if N[fi]@f<=0: continue
            p=P[tri]; x0,x1=int(max(p[:,0].min(),0)),int(min(p[:,0].max()+1,Wd-1)); y0,y1=int(max(p[:,1].min(),0)),int(min(p[:,1].max()+1,Hd-1))
            if x1<x0 or y1<y0: continue
            xs,ys=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5)
            d=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
            if abs(d)<1e-9: continue
            w0=((p[1,1]-p[2,1])*(xs-p[2,0])+(p[2,0]-p[1,0])*(ys-p[2,1]))/d
            w1=((p[2,1]-p[0,1])*(xs-p[2,0])+(p[0,0]-p[2,0])*(ys-p[2,1]))/d
            w2=1-w0-w1; inside=(w0>=-1e-4)&(w1>=-1e-4)&(w2>=-1e-4)
            z=w0*p[0,2]+w1*p[1,2]+w2*p[2,2]
            sub=zb[y0:y1+1,x0:x1+1]; ok=inside&(z>sub)
            sub[ok]=z[ok]; img[y0:y1+1,x0:x1+1][ok]=np.array(col)*sh[fi]
    return Image.fromarray(img.clip(0,255).astype('uint8')).resize(size,Image.LANCZOS)
GRAF=(95,100,110); NAR=(235,120,30); MET=(170,175,182); VER=(30,120,70); NEG=(20,60,140)
base=[(man,MET),(cuerpo,GRAF),(pcb,VER),(lcd,NEG)]
desp=tapa.copy(); n=np.array([0,-np.sin(ANG),np.cos(ANG)]); desp.apply_translation(n*16)
v1=render(base+[(tapa,NAR)],az=-28,el=38)
v2=render(base+[(tapa,NAR)],az=-90,el=0)
v3=render(base+[(tapa,NAR)],az=-75,el=18,center=(0,-3,24))
out=Image.new('RGB',(620*3,600),(28,30,34))
dr=ImageDraw.Draw(out); ft=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',20)
for i,(v,t) in enumerate([(v1,'Vista desde el piloto'),(v2,'Vista lateral (45°)'),(v3,'Lado izquierdo: hueco del USB')]):
    out.paste(v,(620*i,40)); dr.text((620*i+310,22),t,font=ft,fill=(230,230,230),anchor='mm')
out.save('docs/img/soporte.png')
