/*
 * Adapted from EMU145 by Felix Lazarev.
 * Emulator code GPL-3.0; ROM copyright notice retained in NOTICE.
 */
#include "microcode_chip.hpp"

namespace pmk {

const unsigned char jrom[42]={
    0,1,2,3,4,5,3,4,5,3,4,5,3,4,5,3,4,5,3,4,5,3,4,5,6,7,8,
    0,1,2,3,4,5,6,7,8,
    0,1,2,3,4,5
};

void MicrocodeChip::init()
{
    icount=0;
    dcount=0;
    ecount=0;
    ucount=0;
    sigma=0;
    carry=0;
    rl=0;
    rt=0;
    was_t_qrd=false;

    cptr=0;
    command=cmdrom[cptr];
}
void MicrocodeChip::pretick(bool rin)
{
    rm[MCU_BITLEN-1]=rin;
}

bool MicrocodeChip::tick(bool rin,bool k1, bool k2, unsigned int * dcycle, bool * syncout, unsigned char *segment)
{
    unsigned i;
    unsigned int ret;
    int a;
    int b;
    int g;
    int newr0 = rr[0];
    int newm0;
    bool x,y,z;
    unsigned char ucmd;
    bool temp;

    a=0;
    b=0;
    g=0;

    //rm[MCU_BITLEN-1]=rin; //shift in the data
    if(icount<27)
    {
        ucmd=asprom[command&0x7f][jrom[icount]];
        asp=command&0x7f;
    }else
        if((icount>=27)&&(icount<36))
        {
            ucmd=asprom[(command>>8)&0x7f][jrom[icount]];
            asp=(command>>8)&0x7f;
        }
        else
        {
            // special case
            if(((command>>16)&0xff)>=0x20)
            {
                //r0 points to i=36
                //we need to store ASP field to R1[D14:D13]
                if(icount==36)
                {
                    rr[4*1]=((((command>>16)&0xf)>>ucount)&1)?true:false;
                    rr[4*4]=((((command>>20)&0xf)>>ucount)&1)?true:false;
                }
                ucmd=asprom[0x5f][jrom[icount]];
                asp=0x5f;
            }
            else
            {
                ucmd=asprom[(command>>16)&0x3f][jrom[icount]];
                asp=(command>>16)&0x3f;
            }
        }

    ucmd&=0x3f; //fool's proof
    if(ucmd>0x3b)
    {
        ucmd=(ucmd-0x3c)*2;
        ucmd+=!rl?1:0;
        ucmd+=0x3c;
    }
    cur_ucmd=ucmd;
    u_command=ucrom[ucmd];

    switch(u_command.s1())
    {
        case 2:
            rh[0]=((((k2?1:0)<<3|(k1?1:0))>>ucount)&1)?true:false;
            //rh[0]=(k1|k2)?true:false;
            //temp=rs1[0];
            rs1[0]|=rh[0];
            break;
        case 3:
            rh[0]=((((k2?1:0)<<3|(k1?1:0))>>ucount)&1)?true:false;
            //rh[0]=(k1|k2)?true:false;
            //temp=rs1[0];
            rs1[0]|=rh[0];
            break;
    }

    if(k1|k2)
    {
       // rt=true;
        latchk1=k1;
        latchk2=k2;
    }
    if(u_command.g_nt())
        was_t_qrd=true;
    if(latchk1|latchk2)
    {
        rt=true;
    }
    else
        rt=false;
    //if((command&0xfc0000)==0)
    {
        if(u_command.g_nt() | was_t_qrd)
            rs1[0]=((((latchk2?1:0)<<3|(latchk1?1:0))>>ucount)&1)?true:false;
    }


    if(u_command.a_r())
        a|=rr[0];

    if(u_command.a_m())
        a|=rm[0];

    if(u_command.a_st())
        a|=rst[0];

    if(u_command.a_nr())
        a|=!rr[0];

    if(u_command.a_10nl())
        a|=((10>>ucount)&1)&!rl;

    if(u_command.a_s())
        a|=rs[0];
    if(u_command.a_4())
        a|=(4>>ucount)&1;





    if(u_command.b_1())
        b|=(1>>ucount)&1;
    if(u_command.b_6())
        b|=(6>>ucount)&1;
    if(u_command.b_s())
        b|=rs[0];
    if(u_command.b_s1())
        b|=rs1[0];
    if(u_command.b_ns())
        b|=!rs[0];

    if(u_command.g_l())
        g|=rl;
    if(u_command.g_nl())
        g|=!rl;



    if(u_command.g_nt())
         g|=!rt;

    if(ucount!=0) //gamma input -- 1 bit data or carry only.
        g=carry;

    sigma=a+b+g;
    carry=((sigma>>1)&1)?true:false;
    sigma&=1;

    switch(u_command.r0())
    {
        case 0:
            newr0=rr[0];
            break;
        case 1:
            newr0=rr[3*4];
            break;
        case 2:
            newr0=sigma;
            break;
        case 3:
            newr0=rs[0];
            break;
        case 4:
            newr0=rr[0]|rs[0]|sigma;
            break;
        case 5:
            newr0=rs[0]|sigma;
            break;
        case 6:
            newr0=rr[0]|rs[0];
            break;
        case 7:
            newr0=rr[0]|sigma;
            break;
    }
    if(u_command.r_1())
    {
        if(icount<36)
        {
            if((command&0xff000000)==0)
                rr[MCU_BITLEN-1*4]=sigma;
        }
        else
            rr[MCU_BITLEN-1*4]=sigma;
    }
    if(u_command.r_2())
    {
        if(icount<36)
        {
            if((command&0xff000000)==0)
                rr[MCU_BITLEN-2*4]=sigma;
        }
        else
            rr[MCU_BITLEN-2*4]=sigma;
    }
    if(u_command.l())
        if(ucount==3)
            rl=carry;
    if(u_command.m())
        newm0=rs[0];
    else
        newm0=rm[0];

    switch(u_command.s())
    {
        case 0:
            temp=rs[0];
            rs[0]=rs[1];rs[1]=rs[2];rs[2]=rs[3];
            rs[3]=temp;
            break;
        case 1:
            rs[0]=rs[1];rs[1]=rs[2];rs[2]=rs[3];
            rs[3]=rs1[0];
            break;
        case 2:
            rs[0]=rs[1];rs[1]=rs[2];rs[2]=rs[3];
            rs[3]=sigma;
            break;
        case 3:
            temp=rs1[0];
            rs1[0]=rs1[1];rs1[1]=rs1[2];rs1[2]=rs1[3];
            rs1[3]=sigma|temp;
            break;
    }

    switch(u_command.s1())
    {
        case 0:
            temp=rs1[0];
            rs1[0]=rs1[1];rs1[1]=rs1[2];rs1[2]=rs1[3];
            rs1[3]=temp;
            break;
        case 1:
            rs1[0]=rs1[1];rs1[1]=rs1[2];rs1[2]=rs1[3];
            rs1[3]=sigma;
            break;
#if 1
        case 2:
            temp=rs1[0];
            rs1[0]=rs1[1];rs1[1]=rs1[2];rs1[2]=rs1[3];
            rs1[3]=temp;
            break;
        case 3:
            temp=rs1[0];
            rs1[0]=rs1[1];rs1[1]=rs1[2];rs1[2]=rs1[3];
            rs1[3]=temp|sigma;
            break;
#endif
    }

    switch(u_command.st())
    {
        case 1:
            rst[8]=rst[4];
            rst[4]=rst[0];
            rst[0]=sigma;
            break;
        case 2:
            temp=rst[0];
            rst[0]=rst[4];
            rst[4]=rst[8];
            rst[8]=temp;
            break;
        case 3:
            x=rst[0*4];
            y=rst[1*4];
            z=rst[2*4];
            rst[0*4]=sigma|y;
            rst[1*4]=x|z;
            rst[2*4]=x|y;
            break;
    }


    ret=newm0;
    for(i=0;i<(MCU_BITLEN-1);i++)
        rm[i]=rm[i+1];
    rm[MCU_BITLEN-1]=rin;



    if((icount<36)&&(command&0xff000000))//mod flag -- do not modify R!!!
        newr0=rr[0];

    for(i=0;i<(MCU_BITLEN-1);i++)
        rr[i]=rr[i+1];
    rr[MCU_BITLEN-1]=newr0;

    temp=rh[0];
    rh[0]=rh[1];rh[1]=rh[2];rh[2]=rh[3];
    rh[3]=temp;
    if((dcount<13)&&(ecount==0))
    {
        for(i=0;i<3;i++)
            dispout[i]=dispout[i+1];
        dispout[3]=newr0;
    }

    temp=rst[0];
    for(i=0;i<(MCU_BITLEN-1);i++)
        rst[i]=rst[i+1];
    rst[MCU_BITLEN-1]=temp;

    ucount++;
    if(ucount>=4)
    {
        ucount=0;
        icount++;
        ecount++;
    }
    if(icount>=42)
    {
        icount=0;
        cptr=rr[39*4+0]|rr[39*4+1]<<1|rr[39*4+2]<<2|rr[39*4+3]<<3;
        cptr=cptr<<4|rr[36*4+0]|rr[36*4+1]<<1|rr[36*4+2]<<2|rr[36*4+3]<<3;
        command=cmdrom[cptr];
        was_t_qrd=false;
        rt=false;
        latchk1=false;
        latchk2=false;
    }
    if(ecount>=3)
    {
        ecount=0;
        dcount++;
     }
    if(dcount>=14)
    {
        dcount=0;
    }

    //only needed for master - 1302
    if((dcycle)&&(syncout))
    {
        *dcycle=(((command&0xfc0000)==0)&&(ecount==0)&&(ucount==0))?(dcount+1):0;   //return value is D strobe idx for keyboard/display scan
        *syncout=((dcount==13)&&(ecount==2)&&(ucount==3))?true:false;
    }

    if(segment)
    {
        *segment=(((command&0xfc0000)==0)&&(ecount==0)&&(ucount==0))?(dispout[0]|dispout[1]<<1|dispout[2]<<2|dispout[3]<<3):0;
        temp=((command&0xfc0000)==0)?rl:0;
        *segment|=(temp)?0x80:0;
    }
    return (ret&1)?true:false;
}

bool MicrocodeChip::strobe()
{
    return (((command&0xfc0000)==0))?true:false;   //return value is D strobe idx for keyboard/display scan

}

std::uint8_t MicrocodeChip::memoryNibble(unsigned index) const {
    std::uint8_t value = 0;
    for (unsigned bit = 0; bit < 4; ++bit)
        if (rm[index * 4 + bit])
            value |= static_cast<std::uint8_t>(1U << bit);
    return value;
}

void MicrocodeChip::setMemoryNibble(unsigned index, std::uint8_t value) {
    for (unsigned bit = 0; bit < 4; ++bit)
        rm[index * 4 + bit] = (value & (1U << bit)) != 0;
}

} // namespace pmk

