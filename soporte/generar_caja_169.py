"""Caja desmontable para la placa Waveshare ESP32-S3-LCD-1.69 con GPS y botonera
(ejecutar desde la raiz del repositorio). Genera STL para imprimir y STEP para Fusion.

Tres piezas:
  - base:    se queda en el manillar (bridas). Lleva el carril en cola de milano y la pestana de cierre.
  - modulo:  placa a la izquierda, GPS y antena a la derecha, y debajo el hueco de los cables.
  - tapa:    ventana de la pantalla, asiento de la botonera y ranura para su cable plano.

Coordenadas locales del modulo, visto desde el piloto: u = ancho, v = alto, w = fondo (0 = trasera).
Todas las medidas en mm.
"""
import numpy as np, cadquery as cq, trimesh, trimesh.repair, os, tempfile
from PIL import Image, ImageDraw, ImageFont

def B(x0,x1,y0,y1,z0,z1): return cq.Workplane("XY").box(x1-x0,y1-y0,z1-z0,centered=False).translate((x0,y0,z0))
def cylZ(r,z0,z1,x=0,y=0): return cq.Workplane("XY").circle(r).extrude(z1-z0).translate((x,y,z0))
def cylX(r,x0,x1,y=0,z=0): return cq.Workplane("YZ").circle(r).extrude(x1-x0).translate((x0,y,z))
def prismaV(pts_uw,v0,v1): return cq.Workplane("XZ").polyline(pts_uw).close().extrude(v1-v0).translate((0,v1,0))
def prismaU(pts_vw,u0,u1): return cq.Workplane("YZ").polyline(pts_vw).close().extrude(u1-u0).translate((u0,0,0))
def U(l):
    r=l[0]
    for m in l[1:]: r=r.union(m)
    return r
def D(a,l):
    for m in l: a=a.cut(m)
    return a

# ---------------- medidas ----------------
RB=14.3; GOMA=0.7                 # manillar de 28,6 mm (sin medir en la moto) y holgura para goma
TAPA=2.4
FONDO=17.0                        # fondo del modulo sin tapa
U0,U1=-35.0,35.0; V0,V1=-34.0,36.5
# placa: 30 x 37,12; del cristal a la cara trasera 3,14; del cristal a lo alto del USB 6,40 (calibre)
ZG=FONDO-0.1                      # cara del cristal, casi tocando la tapa
ZB=ZG-3.14                        # cara trasera de la placa: apoya en los cuatro pilares
SUELO_PLACA=ZG-6.40-0.6
PL_U0,PL_U1=-32.0,-2.0            # placa (el USB queda a la izquierda, visto de frente)
PL_V1=31.2; PL_V0=PL_V1-37.12
PCU,PCV=(PL_U0+PL_U1)/2,(PL_V0+PL_V1)/2
TAL=[(PCU+a,PCV+b) for a in (-13,13) for b in (-14.5,14.5)]      # taladros de 1,5 mm
USB_V=PCV-0.85                    # centro del USB (del plano del fabricante)
# zona visible de la pantalla: 27,97 x 32,63, esquinas de radio 5
VIS_U,VIS_V=27.97,32.63; VIS_ARRIBA=1.5                      # margen supuesto desde el borde superior de la placa
VCV=PL_V1-VIS_ARRIBA-VIS_V/2
# GPS: placa de 26,41 x 35,44 x 4,03 con la antena de 25,1 x 25,1 x 9,0 encima
GU0,GU1=5.0,32.0; GV0,GV1=-4.5,31.5; SUELO_GPS=1.5
# hueco de los cables (25 mm hasta que se pueden doblar)
CV0,CV1=-31.5,PL_V0+0.02; SUELO_CAB=6.2
# botonera de 40 x 20 pegada sobre la tapa; cable plano de 9,5 x 0,3 con conector en la punta
BOT_U,BOT_V=0.0,-19.0
# cola de milano (hembra en el modulo, macho en la base), bajo la placa
CU=-15.0
CM_BOCA,CM_FONDO,CM_ALTO=12.0,18.0,5.2; CM_FIN=V1-10.4; HOLG=0.3
PU0,PU1=CU+11.0,CU+16.0            # pestana de cierre
TORN=[(-31.0,34.0),(2.0,34.0),(31.0,34.0),(2.0,-1.0),(-31.5,-30.5),(31.5,-30.5)]   # tornillos de la tapa

# ---------------- modulo ----------------
ext=B(U0,U1,V0,V1,0,FONDO).edges("|Z").fillet(3.0)
cav_placa=B(PL_U0-0.5,PL_U1+1.5,PL_V0,PL_V1+0.3,SUELO_PLACA,FONDO+1)
cav_gps=B(GU0,GU1,GV0,GV1,SUELO_GPS,FONDO+1)
cav_cab=B(U0+2.5,U1-3.0,CV0,CV1,SUELO_CAB,FONDO+1)
paso_gps=B(11,26,CV1-0.1,GV0+0.1,SUELO_CAB,FONDO+1)                # cables del GPS hacia el hueco de cables
usb=B(U0-1,PL_U0+0.5,USB_V-7,USB_V+7,ZB-1.63-4.2,FONDO+1)         # entrada del cable USB-C
def trap(c): return [(c-CM_BOCA/2,-0.1),(c+CM_BOCA/2,-0.1),(c+CM_FONDO/2,CM_ALTO),(c-CM_FONDO/2,CM_ALTO)]
ranura=prismaV(trap(CU),V0-1,CM_FIN)
seguro_mod=cylX(1.7,U0-1,CU+CM_FONDO/2+1,y=-12,z=2.6)             # tornillo M3 de seguridad
pilotos=[cylZ(0.9,8,FONDO+1,x=x,y=y) for x,y in TORN]
esquinas=[B(U0+2.4,U0+7.5,CV0-0.1,CV0+6,SUELO_CAB-0.1,FONDO),B(U1-8,U1-2.9,CV0-0.1,CV0+6,SUELO_CAB-0.1,FONDO)]
pilares=[cylZ(1.7,SUELO_PLACA-0.1,ZB,x=x,y=y) for x,y in TAL]
tetones=[cylZ(0.62,ZB-0.1,ZB+1.3,x=x,y=y) for x,y in TAL]          # entran en los taladros y centran la placa
modulo=D(ext,[cav_placa,cav_gps,cav_cab,paso_gps,usb,ranura,seguro_mod])
modulo=D(U([modulo]+esquinas+pilares+tetones),pilotos)

# ---------------- tapa ----------------
def rect_r(cu,cv,a,b,r,z0,z1): return cq.Workplane("XY").rect(a,b).extrude(z1-z0).edges("|Z").fillet(r).translate((cu,cv,z0))
vent=rect_r(PCU,VCV,VIS_U+1.2,VIS_V+1.2,4.6,FONDO-0.5,FONDO+TAPA+0.5)
bisel=(cq.Workplane("XY").workplane(offset=FONDO+0.9).rect(VIS_U+1.2,VIS_V+1.2)
       .workplane(offset=TAPA-0.9+0.01).rect(VIS_U+4.4,VIS_V+4.4).loft(combine=True).translate((PCU,VCV,0)))
asiento=B(BOT_U-20.3,BOT_U+20.3,BOT_V-10.3,BOT_V+10.3,FONDO+TAPA-0.4,FONDO+TAPA+1)      # la botonera queda enrasada y centrada
paso_cinta=B(BOT_U+20.6,BOT_U+23.8,BOT_V-5.6,BOT_V+5.6,FONDO-1,FONDO+TAPA+1)          # pasa el conector y el cable plano
tapa=D(B(U0,U1,V0,V1,FONDO,FONDO+TAPA).edges("|Z").fillet(3.0),
       [vent,bisel,asiento,paso_cinta]+[cylZ(1.2,FONDO-1,FONDO+TAPA+1,x=x,y=y) for x,y in TORN])

# ---------------- base ----------------
PF=-0.25                                              # cara delantera de la placa de la base
P_V1=V1-6.4
placa=B(CU-10,CU+10,V0,P_V1,PF-5,PF)
def trapb(c): return [(c-(CM_BOCA/2-HOLG),PF),(c+CM_BOCA/2-HOLG,PF),(c+CM_FONDO/2-HOLG,CM_ALTO-HOLG),(c-(CM_FONDO/2-HOLG),CM_ALTO-HOLG)]
carril=prismaV(trapb(CU),V0+2.4,CM_FIN-HOLG)
pie_carril=B(CU-(CM_BOCA/2-HOLG),CU+CM_BOCA/2-HOLG,V0+2.4,CM_FIN-HOLG,PF-1,PF+0.2)
brazo=B(PU0,PU1,V0,V1+9.6,-3.2,-0.8)
raiz=B(CU+9,PU1,V0,V0+5.4,PF-5,-0.8)
gancho=prismaU([(V1+0.5,-0.8),(V1+0.5,3.0),(V1+1.2,3.0),(V1+4.1,-0.8)],PU0,PU1)
hueco_brazo=B(PU0-0.6,PU1+0.6,V0+5.9,V1+11.6,PF-10,PF+0.01)
seguro_base=cylX(1.7,CU-12,CU+12,y=-12,z=2.6)

# colocacion en la moto: pantalla a 45 grados hacia el piloto, modulo centrado sobre el manillar
ANG=45.0; OY,OZ=3.0,46.0
def W(m): return m.rotate((0,0,0),(1,0,0),ANG).translate((0,OY,OZ))        # local -> moto
def Lc(m): return m.translate((0,-OY,-OZ)).rotate((0,0,0),(1,0,0),-ANG)    # moto -> local
def Wp(p):
    a=np.radians(ANG); u,v,w=p
    return cq.Vector(u, OY+v*np.cos(a)-w*np.sin(a), OZ+v*np.sin(a)+w*np.cos(a))

silla=B(-17,17,-14,14,3,21)
barra=cylX(RB+GOMA,-40,40)
tuneles=[B(x-2.9,x+2.9,-30,30,16.3,18.9) for x in (-11,11)]
wA=cq.Wire.makePolygon([cq.Vector(*p) for p in [(-17,-14,21),(17,-14,21),(17,14,21),(-17,14,21)]],close=True)
wB=cq.Wire.makePolygon([Wp(p) for p in [(CU-10,V0,PF-5),(CU+10,V0,PF-5),(CU+10,P_V1,PF-5),(CU-10,P_V1,PF-5)]],close=True)
cuello=cq.Workplane("XY").add(cq.Solid.makeLoft([wA,wB],True))
base_m=D(U([cuello,silla,W(placa)]),[barra]+tuneles+[W(hueco_brazo)])
base_m=U([base_m,W(carril),W(pie_carril),W(brazo),W(raiz),W(gancho)])
base_m=D(base_m,[W(seguro_base)])
base=Lc(base_m)

# ---------------- exportar ----------------
DIR='soporte/caja_169/'
def malla(m):
    f=tempfile.mktemp(suffix='.stl'); cq.exporters.export(m,f,tolerance=0.02,angularTolerance=0.15)
    t=trimesh.load(f); os.remove(f); t.merge_vertices(digits_vertex=3)
    if not t.is_watertight: trimesh.repair.fill_holes(t)
    return t
def suelo(m): return m.translate((0,0,-m.val().BoundingBox().zmin))
piezas={'caja169_modulo':suelo(modulo),'caja169_tapa':suelo(tapa),'caja169_base':suelo(base)}   # posturas de impresion
for n,m in piezas.items():
    cq.exporters.export(m,DIR+n+'.step')
    malla(cq.importers.importStep(DIR+n+'.step')).export(DIR+n+'.stl')   # desde el STEP: la malla sale cerrada
conj=cq.Assembly()
conj.add(W(modulo),name='modulo',color=cq.Color(0.37,0.39,0.43)); conj.add(W(tapa),name='tapa',color=cq.Color(0.92,0.47,0.12))
conj.add(base_m,name='base',color=cq.Color(0.6,0.62,0.66))
conj.save(DIR+'caja169_conjunto.step')

mm,mt,mb=malla(modulo),malla(tapa),malla(base)
print('estancos:',mm.is_watertight,mt.is_watertight,mb.is_watertight,'| cuerpos:',[len(x.val().Solids()) if hasattr(x.val(),'Solids') else 1 for x in (modulo,tapa,base)])
print('modulo',mm.extents.round(1),'| tapa',mt.extents.round(1),'| base',mb.extents.round(1))
choque=base.intersect(U([modulo,tapa])); print('interferencia base-modulo: %.2f mm3'%choque.val().Volume())
bar=W(modulo).union(W(tapa)).intersect(cylX(RB+GOMA+3,-60,60)); print('interferencia modulo-manillar (+3 mm): %.2f mm3'%bar.val().Volume())

# ---------------- vista previa ----------------
def render(objs,az,el,size=(620,560),scale=5.2,center=(0,0,30)):
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
    return Image.fromarray(img.clip(0,255).astype('uint8')).resize(size,Image.LANCZOS)


GRAF=(95,100,110); NAR=(235,120,30); MET=(170,175,182); VER=(30,120,70); AZ=(20,60,140); ROSA=(215,170,150); CLARO=(150,156,166); AMA=(240,200,40); ROJ=(210,50,40)
man=malla(cylX(RB,-70,70))
pcb=B(PL_U0,PL_U1,PL_V0,PL_V1,ZB,ZB+1.6); lcd=B(PL_U0,PL_U1,PL_V0+1,PL_V1,ZB+1.6,ZG)
gps=B(GU0+0.3,GU1-0.3,GV0+0.3,GV1-0.3,SUELO_GPS,SUELO_GPS+4.0); ant=B(6,31.1,0,25.1,SUELO_GPS+4.2,SUELO_GPS+13.2)
zt=FONDO+TAPA
bot=B(BOT_U-20,BOT_U+20,BOT_V-10,BOT_V+10,zt-0.4,zt+0.4); b1=cylZ(6.5,zt+0.4,zt+0.9,x=BOT_U-9,y=BOT_V); b2=cylZ(6.5,zt+0.4,zt+0.9,x=BOT_U+9,y=BOT_V)
def conjunto(sube=0,con_tapa=True):
    mv=lambda m: malla(W(m.translate((0,sube,0))))
    o=[(man,MET),(malla(base_m),CLARO),(mv(modulo),GRAF),(mv(pcb),VER),(mv(lcd),AZ),(mv(gps),AZ),(mv(ant),ROSA)]
    if con_tapa: o+=[(mv(tapa),NAR),(mv(bot),(60,60,66)),(mv(b1),ROJ),(mv(b2),AMA)]
    return o
C=(0,0,48)
v1=render(conjunto(),az=-28,el=38,center=C,scale=4.2)
v2=render(conjunto(con_tapa=False),az=-28,el=38,center=C,scale=4.2)
v3=render(conjunto(sube=40),az=-150,el=20,center=(0,8,55),scale=3.6)
v4=render(conjunto(),az=-90,el=0,center=C,scale=4.2)
out=Image.new('RGB',(620*2,600*2),(28,30,34)); dr=ImageDraw.Draw(out)
ft=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',20)
for i,(v,t) in enumerate([(v1,'Montado, visto desde el piloto'),(v2,'Sin tapa: placa a la izquierda, GPS a la derecha, cables abajo'),
                          (v3,'Por detras, sacando el modulo: carril y pestana'),(v4,'Vista lateral (45°)')]):
    x,y=620*(i%2),600*(i//2); out.paste(v,(x,y+40)); dr.text((x+310,y+22),t,font=ft,fill=(230,230,230),anchor='mm')
out.save('docs/img/caja_169.png')

