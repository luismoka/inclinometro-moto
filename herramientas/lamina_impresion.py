"""Dibuja las piezas de la caja con GPS tal como van sobre la cama de impresion."""
import numpy as np, trimesh
from PIL import Image, ImageDraw, ImageFont
from trimesh.creation import box

def render(objs,az,el,size,scale,center):
    SS=2; Wd,Hd=size[0]*SS,size[1]*SS
    a,e=np.radians(az),np.radians(el)
    f=np.array([np.cos(e)*np.sin(a),-np.cos(e)*np.cos(a),np.sin(e)])
    r=np.cross([0,0,1],f); r/=np.linalg.norm(r); u=np.cross(f,r)
    img=np.zeros((Hd,Wd,3))+np.array([28,30,34]); zb=np.full((Hd,Wd),-1e9)
    luz=np.array([-0.35,-0.5,0.8]); luz/=np.linalg.norm(luz)
    for m,col in objs:
        V=m.vertices-np.array(center); P=np.c_[V@r*scale*SS+Wd/2, Hd/2-V@u*scale*SS, V@f]
        N=m.face_normals; sh=0.35+0.65*np.clip(N@luz,0,1)
        for fi,t in enumerate(m.faces):
            if N[fi]@f<=0: continue
            p=P[t]; x0,x1=int(max(p[:,0].min(),0)),int(min(p[:,0].max()+1,Wd-1)); y0,y1=int(max(p[:,1].min(),0)),int(min(p[:,1].max()+1,Hd-1))
            if x1<x0 or y1<y0: continue
            xs,ys=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5)
            d=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
            if abs(d)<1e-9: continue
            w0=((p[1,1]-p[2,1])*(xs-p[2,0])+(p[2,0]-p[1,0])*(ys-p[2,1]))/d
            w1=((p[2,1]-p[0,1])*(xs-p[2,0])+(p[0,0]-p[2,0])*(ys-p[2,1]))/d
            w2=1-w0-w1; ok=(w0>=-1e-4)&(w1>=-1e-4)&(w2>=-1e-4)
            z=w0*p[0,2]+w1*p[1,2]+w2*p[2,2]
            sub=zb[y0:y1+1,x0:x1+1]; ok&=z>sub
            sub[ok]=z[ok]; img[y0:y1+1,x0:x1+1][ok]=np.array(col)*sh[fi]
    def px(pt):
        v=np.array(pt)-np.array(center); return (Wd/2+v@r*scale*SS)/SS,(Hd/2-v@u*scale*SS)/SS
    return Image.fromarray(img.clip(0,255).astype('uint8')).resize(size,Image.LANCZOS),px

PIEZAS=[('gps_base.stl','1 · Base (se queda en la moto)','con soportes',(200,205,212)),
        ('gps_modulo.stl','2 · Módulo','sin soportes',(110,116,128)),
        ('gps_tapa.stl','3 · Tapa','sin soportes',(235,120,30)),
        ('soporte_pulsadores_x3.stl','4 · Pulsadores (3)','sin soportes',(60,200,110))]
objs=[]; marcas=[]; x=0
for f,nom,sop,col in PIEZAS:
    m=trimesh.load('soporte/'+f); b=m.bounds
    m.apply_translation([x-b[0][0],-(b[0][1]+b[1][1])/2,-b[0][2]])
    objs.append((m,col)); e=m.extents
    marcas.append(((x+e[0]/2,-e[1]/2-4,0),nom,'%.0f × %.0f × %.0f mm · %s'%(e[0],e[1],e[2],sop)))
    x+=e[0]+16
cama=box(extents=[x+14,96,1]); cama.apply_translation([x/2-8,0,-0.55])
size=(1500,760)
img,px=render([(cama,(52,56,64))]+objs,az=-8,el=40,size=size,scale=5.0,center=(x/2-8,-10,6))
d=ImageDraw.Draw(img)
F='/usr/share/fonts/truetype/dejavu/DejaVuSans'
f1=ImageFont.truetype(F+'-Bold.ttf',21); f2=ImageFont.truetype(F+'.ttf',17); f3=ImageFont.truetype(F+'-Bold.ttf',26)
d.text((size[0]/2,30),'Caja con GPS: piezas a imprimir, en su postura sobre la cama',font=f3,fill=(235,235,235),anchor='mm')
for pt,nom,det in marcas:
    X,Y=px(pt); d.text((X,Y+22),nom,font=f1,fill=(240,240,240),anchor='mm'); d.text((X,Y+48),det,font=f2,fill=(170,176,186),anchor='mm')
d.text((size[0]/2,size[1]-22),'PETG · capa 0,2 mm · 3–4 perímetros · 30–40 % de relleno',font=f2,fill=(170,176,186),anchor='mm')
img.save('docs/img/caja_gps_piezas.png')
