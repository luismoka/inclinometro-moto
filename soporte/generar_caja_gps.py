"""Caja desmontable con GPS para el inclinometro (ejecutar desde la raiz del repositorio).

Tres piezas, mas los pulsadores de soporte_pulsadores_x3.stl:
  - base:    se queda en el manillar (bridas). Lleva el carril en cola de milano y la pestana de cierre.
  - modulo:  placa + GPS + antena. Entra deslizando de arriba abajo en el carril.
  - tapa:    cierra el modulo (ventana de pantalla y agujeros de los botones).

Coordenadas locales del modulo: u = ancho (botones a la izquierda), v = alto, w = fondo (0 = trasera).
Todas las medidas en mm.
"""
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
def prismaU(pts_vw,u0,u1):
    """Prisma convexo: seccion dada en el plano (v,w), extruida entre u0 y u1."""
    p=np.array([[u,v,w] for u in (u0,u1) for v,w in pts_vw]); return trimesh.convex.convex_hull(p)
def prismaV(pts_uw,v0,v1):
    p=np.array([[u,v,w] for v in (v0,v1) for u,w in pts_uw]); return trimesh.convex.convex_hull(p)
U=lambda l: trimesh.boolean.union(l,engine='manifold')
D=lambda a,l: trimesh.boolean.difference([a]+l,engine='manifold')

# ---------------- medidas (calibre) ----------------
RB=14.3; GOMA=0.7                 # manillar de 28,6 mm (sin medir en la moto) y holgura para goma
TAPA=2.4
FONDO=17.0                        # fondo del modulo sin tapa
HUECO_PLACA=7.45                  # USB 3,35 + placa 1,7 + cristal 1,85 + 0,5 de holgura
SUELO_PLACA=FONDO-HUECO_PLACA
# modulo: exterior
U0,U1=-22.0,51.0; V0,V1=-20.4,20.4
UC=(U0+U1)/2                      # centro del modulo: queda sobre el centro del manillar
# zona de la placa principal (34,50 x 25,43)
PU,PV=17.6,13.1
# zona del GPS: placa de 26,41 x 35,44 x 4,03 con la antena de 25,1 x 25,1 x 9,0 encima
GU0,GU1=19.6,46.6; GV=18.0; SUELO_GPS=2.0
# cola de milano (hembra en el modulo, macho en la base), centrada bajo la placa principal
CM_BOCA,CM_FONDO,CM_ALTO=12.0,18.0,5.2; CM_FIN=10.0; HOLG=0.3
# pestana de cierre
PU0,PU1=11.0,16.0

# ---------------- modulo ----------------
ext=B(U0,U1,V0,V1,0,FONDO)
cav_placa=U([B(-PU,PU,-PV,PV,SUELO_PLACA,FONDO+1), B(-16.5,-9,12,14.4,SUELO_PLACA,FONDO+1)])
cav_gps=B(GU0,GU1,-GV,GV,SUELO_GPS,FONDO+1)
paso_cables=B(PU-0.1,GU0+0.1,-6,6,SUELO_PLACA,FONDO+1)
usb=B(U0-1,-15,-5.5,7.5,SUELO_PLACA-1.0,SUELO_PLACA+5.5)
ranura=prismaV([(-CM_BOCA/2,-0.1),(CM_BOCA/2,-0.1),(CM_FONDO/2,CM_ALTO),(-CM_FONDO/2,CM_ALTO)],V0-1,CM_FIN)
TORN=[(-19.8,-17.5),(-19.8,17.5),(48.8,-17.5),(48.8,17.5)]
pilotos=[cylZ(0.9,8,FONDO+1,x=x,y=y) for x,y in TORN]
seguro_mod=cylX(1.7,U0-1,CM_FONDO/2+1,y=-6,z=2.6,s=32)       # agujero para tornillo M3 de seguridad
# apoyos en el extremo de la pantalla, a la altura del conector USB, para que la placa quede recta
apoyos=[B(14.4,17.7,10.2,13.2,SUELO_PLACA-0.2,SUELO_PLACA+3.4),B(14.4,17.7,-13.2,-10.2,SUELO_PLACA-0.2,SUELO_PLACA+3.4)]
modulo=U([D(ext,[cav_placa,cav_gps,paso_cables,usb,ranura,seguro_mod]+pilotos)]+apoyos)

# ---------------- tapa ----------------
REBAJE=0.73   # rebaje interior sobre los botones para el ala de los pulsadores
vent=trimesh.convex.convex_hull(np.array([[u,v,w] for w,e in ((FONDO-0.5,0.0),(FONDO+TAPA+0.5,2.0)) for u in (-8.5-e,8.0+e) for v in (-8.55-e,7.95+e)]))
tapa=D(B(U0,U1,V0,V1,FONDO,FONDO+TAPA),
       [vent,B(-17.6,-10.9,-11,11,FONDO-0.5,FONDO+REBAJE)]+
       [cylZ(1.2,FONDO-1,FONDO+TAPA+1,x=x,y=y) for x,y in TORN]+
       [cylZ(2.5,FONDO-1,FONDO+TAPA+1,x=-14.35,y=by) for by in (7.1,0.0,-7.1)])

# ---------------- base ----------------
PF=-0.25                                              # cara delantera de la placa de la base
placa=B(-10,10,V0,14,PF-5,PF)
carril=prismaV([(-(CM_BOCA/2-HOLG),PF),(CM_BOCA/2-HOLG,PF),(CM_FONDO/2-HOLG,CM_ALTO-HOLG),(-(CM_FONDO/2-HOLG),CM_ALTO-HOLG)],-18,CM_FIN-HOLG)
pie_carril=B(-(CM_BOCA/2-HOLG),CM_BOCA/2-HOLG,-18,CM_FIN-HOLG,PF-1,PF+0.2)   # solapa carril y placa para que queden unidos
brazo=B(PU0,PU1,V0,30,-3.2,-0.8)
raiz=B(9,PU1,V0,-15,PF-5,-0.8)
gancho=prismaU([(V1+0.5,-0.8),(V1+0.5,3.0),(V1+1.2,3.0),(V1+4.1,-0.8)],PU0,PU1)
hueco_brazo=B(PU0-0.6,PU1+0.6,-14.5,32,PF-10,PF+0.01)   # sitio para que la pestana flexe hacia atras
seguro_base=cylX(1.7,-12,12,y=-6,z=2.6,s=32)

# colocacion en la moto: pantalla a 45 grados hacia el piloto, modulo centrado sobre el manillar
ANG=np.radians(45); OY,OZ=3.0,41.0
T=tr([-UC,OY,OZ])@rot(ANG,[1,0,0])
Ti=np.linalg.inv(T)
def W(m): m=m.copy(); m.apply_transform(T); return m      # local -> moto
def Lc(m): m=m.copy(); m.apply_transform(Ti); return m    # moto -> local

silla=B(-17,17,-14,14,3,21)
losa=B(-17,17,-14,14,19,21)
barra=cylX(RB+GOMA,-40,40)
tuneles=[B(x-2.9,x+2.9,-30,30,16.3,18.9) for x in (-11,11)]
cuello=trimesh.convex.convex_hull(np.vstack([losa.vertices,W(placa).vertices]))
base_m=D(U([cuello,silla]),[barra]+tuneles+[W(hueco_brazo)])
base_m=U([base_m,W(carril),W(pie_carril),W(brazo),W(raiz),W(gancho)])
base_m=D(base_m,[W(seguro_base)])
base=Lc(base_m)                                           # en local: carril hacia arriba, lista para imprimir

# ---------------- exportar (cada pieza en su postura de impresion) ----------------
def exporta(m,nombre):
    m=m.copy(); m.apply_translation([0,0,-m.bounds[0][2]]); m.export('soporte/'+nombre); return m
exporta(modulo,'gps_modulo.stl'); exporta(tapa,'gps_tapa.stl'); exporta(base,'gps_base.stl')
print('estancos:',modulo.is_watertight,tapa.is_watertight,base.is_watertight,
      '| modulo',modulo.extents.round(1),'| base',base.extents.round(1))

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

GRAF=(95,100,110); NAR=(235,120,30); MET=(170,175,182); VER=(30,120,70); AZ=(20,60,140); ROSA=(215,170,150); CLARO=(150,156,166)
man=cylX(RB,-70,70)
pcb=B(-17.25,17.25,-12.7,12.7,SUELO_PLACA+3.4,SUELO_PLACA+5.1); lcd=B(-9.06,12.3,-9.12,8.51,SUELO_PLACA+5.1,SUELO_PLACA+6.95)
gps=B(GU0+0.3,GU1-0.3,-17.7,17.7,SUELO_GPS,SUELO_GPS+4.0); ant=B(20.5,45.6,-12.5,12.6,SUELO_GPS+4.2,SUELO_GPS+13.2)
def conjunto(sube=0,con_tapa=True):
    mv=lambda m: W(m.copy().apply_translation([0,sube,0]))
    o=[(man,MET),(base_m,CLARO),(mv(modulo),GRAF),(mv(pcb),VER),(mv(lcd),AZ),(mv(gps),AZ),(mv(ant),ROSA)]
    if con_tapa: o.append((mv(tapa),NAR))
    return o
v1=render(conjunto(),az=-28,el=38)
v2=render(conjunto(con_tapa=False),az=-28,el=38)
v3=render(conjunto(sube=34),az=-150,el=20,center=(0,8,38),scale=4.6)
v4=render(conjunto(),az=-90,el=0)
out=Image.new('RGB',(620*2,600*2),(28,30,34)); dr=ImageDraw.Draw(out)
ft=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',20)
for i,(v,t) in enumerate([(v1,'Montado, visto desde el piloto'),(v2,'Sin tapa: placa a la izquierda, GPS y antena a la derecha'),
                          (v3,'Por detras, sacando el modulo: carril y pestana'),(v4,'Vista lateral (45°)')]):
    x,y=620*(i%2),600*(i//2); out.paste(v,(x,y+40)); dr.text((x+310,y+22),t,font=ft,fill=(230,230,230),anchor='mm')
out.save('docs/img/caja_gps.png')

# ---------------- comprobaciones ----------------
choque=trimesh.boolean.intersection([base,U([modulo,tapa])],engine='manifold')
print('interferencia base-modulo montado: %.2f mm3'%(choque.volume if len(choque.faces) else 0))
