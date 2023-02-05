#ifndef __X_LZ77_H
#define __X_LZ77_H
#include<stdexcept>
#include<string>
#include<vector>
#include<string.h>
#include<stdint.h>
namespace lz77{enum{DEFAULT_SEARCHLEN=12,DEFAULT_BLOCKSIZE=64*1024,SHORTRUN_BITS=3,SHORTRUN_MAX=(1<<SHORTRUN_BITS),MIN_RUN=5};inline void push_vlq_uint(size_t n,std::string&out){while(1){unsigned char c=n&0x7F;size_t q=n>>7;if(q==0){out+=c;break;}out+=(c|0x80);n=q;}}inline size_t substr_run(const unsigned char*ai,const unsigned char*ae,const unsigned char*bi,const unsigned char*be){size_t n=0;while(1){if(*ai!=*bi)break;++n;++ai;++bi;if(ai==ae||bi==be)break;}return n;}inline uint32_t fnv32a(const unsigned char*i,size_t len,uint32_t hash=0x811c9dc5){while(len>0){hash^=(uint32_t)(*i);hash*=(uint32_t)0x01000193;++i;--len;}return hash;}inline void pack_bytes(const unsigned char*i,uint16_t&packed,size_t blocksize){uint32_t a=fnv32a(i,MIN_RUN);packed=a%blocksize;}inline size_t vlq_length(size_t x){size_t ret=1;if(x>0x7F)ret++;if(x>0x3fff)ret++;if(x>0x1fffff)ret++;return ret;}inline size_t gains(size_t run,size_t offset){size_t gain=run;offset=offset<<(SHORTRUN_BITS+1);size_t loss=vlq_length(offset);if(run>=SHORTRUN_MAX){loss+=vlq_length(run-MIN_RUN+1);}if(loss>gain)return 0;return gain-loss;}struct offsets_dict_t{typedef std::vector<size_t>offsets_t;offsets_t offsets;size_t searchlen;size_t blocksize;offsets_dict_t(size_t sl,size_t bs):searchlen(sl),blocksize(bs){offsets.resize((searchlen+1)*blocksize);}void clear(){offsets.assign((searchlen+1)*blocksize,0);}static size_t*prev(size_t*b,size_t*e,size_t*i){if(i==b)i=e;--i;return i;}static size_t push_back(size_t*b,size_t*e,size_t*head,size_t val){*head=val;++head;if(head==e)head=b;return head-b;}void operator()(uint16_t packed,const unsigned char*i0,const unsigned char*i,const unsigned char*e,size_t&maxrun,size_t&maxoffset,size_t&maxgain){size_t*cb_start=&offsets[packed*(searchlen+1)];size_t*cb_beg=(cb_start+1);size_t*cb_end=(cb_start+1+searchlen);size_t*cb_head=cb_beg+*cb_start;size_t*cb_i=cb_head;while(1){cb_i=prev(cb_beg,cb_end,cb_i);if(*cb_i==0)break;size_t pos=*cb_i-1;size_t offset=i-i0-pos;size_t run=substr_run(i,e,i0+pos,e);size_t gain=gains(run,offset);if(gain>maxgain){maxrun=run;maxoffset=offset;maxgain=gain;}if(cb_i==cb_head)break;}*cb_start=push_back(cb_beg,cb_end,cb_head,i-i0+1);}};struct compress_t{offsets_dict_t offsets;compress_t(size_t searchlen=DEFAULT_SEARCHLEN,size_t blocksize=DEFAULT_BLOCKSIZE):offsets(searchlen,blocksize){}std::string feed(const unsigned char*i,const unsigned char*e){const unsigned char*i0=i;std::string ret;std::string unc;push_vlq_uint(e-i,ret);offsets.clear();size_t blocksize=offsets.blocksize;while(i!=e){unsigned char c=*i;if(i>e-MIN_RUN){unc+=c;++i;continue;}size_t maxrun=0;size_t maxoffset=0;size_t maxgain=0;uint16_t packed;pack_bytes(i,packed,blocksize);offsets(packed,i0,i,e,maxrun,maxoffset,maxgain);if(maxrun<MIN_RUN){unc+=c;++i;continue;}if(unc.size()>0){size_t msg=(unc.size()<<1)|1;push_vlq_uint(msg,ret);ret+=unc;unc.clear();}i+=maxrun;maxrun=maxrun-MIN_RUN+1;if(maxrun<SHORTRUN_MAX){size_t msg=((maxoffset<<SHORTRUN_BITS)|maxrun)<<1;push_vlq_uint(msg,ret);}else{size_t msg=(maxoffset<<(SHORTRUN_BITS+1));push_vlq_uint(msg,ret);push_vlq_uint(maxrun,ret);}}if(unc.size()>0){size_t msg=(unc.size()<<1)|1;push_vlq_uint(msg,ret);ret+=unc;unc.clear();}return ret;}std::string feed(const std::string&s){const unsigned char*i=(const unsigned char*)s.data();const unsigned char*e=i+s.size();return feed(i,e);}};struct decompress_t{size_t max_size;std::string ret;unsigned char*out;unsigned char*outb;unsigned char*oute;struct state_t{size_t msg;size_t run;size_t vlq_num;size_t vlq_off;enum{INIT,START,READ_DATA,READ_RUN}state;state_t():msg(0),run(0),vlq_num(0),vlq_off(0),state(INIT){}};state_t state;bool pop_vlq_uint(const unsigned char*&i,const unsigned char*e,size_t&res){while(1){if(i==e)return false;size_t c=*i;if((c&0x80)==0){state.vlq_num|=(c<<state.vlq_off);break;}state.vlq_num|=((c&0x7F)<<state.vlq_off);state.vlq_off+=7;++i;}res=state.vlq_num;state.vlq_num=0;state.vlq_off=0;return true;}decompress_t(size_t _max_size=0):max_size(_max_size),out(NULL),outb(NULL),oute(NULL){}bool feed(const std::string&s,std::string&remaining){const unsigned char*i=(const unsigned char*)s.data();const unsigned char*e=i+s.size();return feed(i,e,remaining);}bool feed(const unsigned char*i,const unsigned char*e,std::string&remaining){if(state.state==state_t::INIT){ret.clear();size_t size;if(!pop_vlq_uint(i,e,size))return true;++i;state=state_t();if(max_size&&size>max_size)throw std::length_error("Uncompressed data in message deemed too large");ret.resize(size);outb=(unsigned char*)ret.data();oute=outb+size;out=outb;state.state=state_t::START;}while(i!=e){if(out==oute){remaining.assign(i,e);state.state=state_t::INIT;return true;}if(state.state==state_t::START){if(!pop_vlq_uint(i,e,state.msg))return false;++i;state.state=((state.msg&1)?state_t::READ_DATA:state_t::READ_RUN);state.msg=state.msg>>1;}if(state.state==state_t::READ_DATA){size_t len=state.msg;if(out+len>oute)throw std::runtime_error("Malformed data while uncompressing");if(i==e)return false;if(i+len>e){size_t l=e-i;::memcpy(out,&(*i),l);out+=l;state.msg-=l;return false;}::memcpy(out,&(*i),len);out+=len;i+=len;state.state=state_t::START;}else if(state.state==state_t::READ_RUN){size_t shortrun=state.msg&(SHORTRUN_MAX-1);if(shortrun){state.run=shortrun;}else{if(!pop_vlq_uint(i,e,state.run))return false;++i;}size_t off=(state.msg>>SHORTRUN_BITS);size_t run=state.run+MIN_RUN-1;unsigned char*outi=out-off;if(outi>=oute||outi<outb||out+run>oute||out+run<out)throw std::runtime_error("Malformed data while uncompressing");if(outi+run<out){::memcpy(out,outi,run);out+=run;}else{while(run>0){*out=*outi;++out;++outi;--run;}}state.state=state_t::START;}}if(out==oute){remaining.assign(i,e);state.state=state_t::INIT;return true;}return false;}std::string&result(){return ret;}};}
#endif