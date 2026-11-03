/* DSOGI估计与公共幅值前馈；两种调制方法都沿用相同前馈定义。 */
#include "imc_estimator.h"
#include <math.h>
#include <string.h>
#ifdef __TI_COMPILER_VERSION__
#pragma CODE_SECTION(IMC_amplitude_feedforward, ".TI.ramfunc")
#endif
void IMC_DSOGI_reset(IMC_DSOGI *s){memset(s,0,sizeof(*s));}
static float clamp(float x,float lo,float hi){return x<lo?lo:(x>hi?hi:x);}
/* 输入三相物理电压，内部Clarke变换、双SOGI正序提取及相角/频率更新，
 * 返回三相估计电压。固定采样周期50 us；修改周期必须同步推导离散系数。
 */
uint16_t IMC_DSOGI_step(IMC_DSOGI *s,const float v[3],float y[3])
{
 const float ts=.00005f,w0=314.159265359f,wn=94.247779608f;
 float ab[2],w,h=.000025f,k=2,det,r0,r1,x,q,c,sn,vd,vq,err,norm,wp;uint16_t j;
 for(j=0;j<3;j++)if(!(v[j]>=-700&&v[j]<=700))return 0;
 ab[0]=(2*v[0]-v[1]-v[2])/3;ab[1]=(v[1]-v[2])*.57735026919f;
 if(!s->initialized){s->theta=atan2f(ab[1],ab[0]);s->amp=sqrtf(ab[0]*ab[0]+ab[1]*ab[1]);s->omega=w0;s->initialized=1;}
 w=s->omega;det=1+h*k*w+h*h*w*w;
 for(j=0;j<2;j++){
  x=s->sogi[0][j];q=s->sogi[1][j];r0=(1-h*k*w)*x-h*w*q+h*k*w*(ab[j]+s->previous[j]);r1=h*w*x+q;
  s->sogi[0][j]=(r0-h*w*r1)/det;s->sogi[1][j]=(h*w*r0+(1+h*k*w)*r1)/det;s->previous[j]=ab[j];
 }
 ab[0]=.5f*(s->sogi[0][0]-s->sogi[1][1]);ab[1]=.5f*(s->sogi[1][0]+s->sogi[0][1]);
 c=cosf(s->theta);sn=sinf(s->theta);vd=ab[0]*c+ab[1]*sn;vq=-ab[0]*sn+ab[1]*c;
 norm=sqrtf(ab[0]*ab[0]+ab[1]*ab[1]);if(norm<1)norm=1;err=vq/norm;
 s->amp+=(ts*1507.964473723f/(1+ts*1507.964473723f))*(fmaxf(vd,0)-s->amp);
 x=s->amp*c;q=s->amp*sn;y[0]=x;y[1]=-.5f*x+.866025403784f*q;y[2]=-.5f*x-.866025403784f*q;
 s->xi=clamp(s->xi+wn*wn*ts*err,-188.4955592f,188.4955592f);wp=clamp(w0+1.41421356237f*wn*err+s->xi,125.6637061f,502.6548246f);
 s->theta+=ts*wp;if(s->theta>=6.28318530718f)s->theta-=6.28318530718f;
 s->omega+=(ts*wn/(1+ts*wn))*(wp-s->omega);return 1;
}
/* 去零序后估计三相幅值；调制比=基准调制比*311.085/估计幅值，限幅0..0.9。
 * amplitude返回物理峰值，raw返回未限幅的调制比，供诊断与保护使用。
 */
float IMC_amplitude_feedforward(const float v[3],float base,float *amplitude,float *raw)
{
 float z=(v[0]+v[1]+v[2])/3,a=v[0]-z,b=v[1]-z,c=v[2]-z;
 *amplitude=sqrtf((2.0f/3.0f)*(a*a+b*b+c*c));
 if(!(*amplitude>=50&&*amplitude<=700)){*raw=0;return 0;}
 *raw=base*311.085197330f/(*amplitude);return clamp(*raw,0,.90f);
}
