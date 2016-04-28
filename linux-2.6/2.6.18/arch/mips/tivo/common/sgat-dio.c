/*
 * General purpose scatter-gather DIO API
 * Please refer to include/linux/sgat-dio.h for the detailed Documentation
 * 
 */
#include <linux/config.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/cacheflush.h>
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif /* LINUX_VERSION_CODE */
#include <linux/sgat-dio.h>
#include <asm/scatterlist.h>

#define sgat_dio_cptouser sgat_dio_cpfrmuser

static void *sgat_dio_malloc(int rqSz);
static void sgat_dio_free(void * buff);
static void expand_sclist(struct sgat_dio_fops *fops,sgat_dio_req *dio_req, int sc_elems);
static void *add_dio_chunk(struct sgat_dio_fops *fops, sgat_dio_req * dio_req, 
                    u_long address, int length,void *sclp);
static int end_dio_req(struct sgat_dio_fops *fops, sgat_dio_req *dio_req);

static void sgat_dio_done(struct sgat_dio_fops *fops, sgat_dio_req *dio_req );
static void sgat_dio_done_nocopy(struct sgat_dio_fops *fops, sgat_dio_req *dio_req);
static int sgat_dio_cpfrmuser(u_char *src, u_char *dest, int size);

static int expand_iobuf(struct iobuf *iobuf, int wanted)
{
	struct page ** maplist;
	
	if (iobuf->array_len >= wanted)
		return 0;
	
	maplist = kmalloc(wanted * sizeof(struct page **), GFP_KERNEL);
	if (unlikely(!maplist))
		return -ENOMEM;

	/* Did it grow while we waited? */
	if (unlikely(iobuf->array_len >= wanted)) {
		kfree(maplist);
		return 0;
	}

	if (iobuf->array_len) {
		memcpy(maplist, iobuf->maplist, iobuf->array_len * sizeof(*maplist));
		kfree(iobuf->maplist);
	}
	
	iobuf->maplist   = maplist;
	iobuf->array_len = wanted;
	return 0;
}

static int iobuf_init(struct iobuf *iobuf)
{
	iobuf->array_len = 0;
	iobuf->nr_pages = 0;
	iobuf->length = 0;
	return expand_iobuf(iobuf, 8);
}
static void iobuf_release(struct iobuf *iobuf)
{
	iobuf->array_len = 0;
	iobuf->nr_pages = 0;
	iobuf->length = 0;
        kfree(iobuf->maplist);
}

static int sgat_dio_cpfrmuser(u_char *src, u_char *dest, int size){
        int i;
        u_char *s = src;
        u_char *d = dest;

        if ( !s || !d )
                return -EINVAL;

        for ( i =0; i < size; i++){
                if (__get_user_nocheck(*d, s, sizeof(u_char))){
                        DIO_DPRINTK("failed verify_area\n");
                        return -EINVAL;
                }
                s++;
                d++;
        }
        return 0;
}
static void *sgat_dio_malloc(int rqSz)
{
        char *resp = NULL;

        if (rqSz <= 0)
                return resp;
        resp = kmalloc(rqSz, GFP_KERNEL);
        if (resp) 
        memset(resp, 0, rqSz);
        DIO_DPRINTK("size=%d,ret=0x%p\n", rqSz, resp);
        return resp;
}

static void sgat_dio_free(void * buff)
{
        if (!buff) 
                return;
        DIO_DPRINTK("Free'ing %p\n",buff);
        kfree(buff);  
}

static void expand_sclist(struct sgat_dio_fops *fops,sgat_dio_req *dio_req, int sc_elems)
{
        int elem_sz = sizeof(struct scatterlist);
        if ( dio_req ){
                if ( !dio_req->sclist || (sc_elems > dio_req->sc_elems) ){
                        if ( dio_req->sclist)
                                fops->sgat_dio_free((void *)dio_req->sclist);
                        dio_req->sclist = fops->sgat_dio_malloc(sc_elems * elem_sz);
                        dio_req->sc_elems = sc_elems;
                }
        }
}

static void *add_dio_chunk(struct sgat_dio_fops *fops, sgat_dio_req * dio_req, 
                    u_long address, int length,void *sclp){
        (void) fops;
        struct  scatterlist *p = (struct  scatterlist *) sclp;
        dio_req->k_useg++;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,13)
        p->page = NULL;
#endif
        p->address = (char *)address;
        p->dma_address = virt_to_phys((char*)address);
        p->length = length;
        p++;
        return p;
}

static int end_dio_req(struct sgat_dio_fops *fops, sgat_dio_req *dio_req) {
        (void) fops;
        dio_req->in_use = 1;
        return 0;
}

static void sgat_dio_done(struct sgat_dio_fops *fops, sgat_dio_req *dio_req )
{
        if (!dio_req )
                return;	
        if ( dio_req->k_io_hdrp &&  dio_req->u_io_hdrp ){
                sgat_dio_cptouser((u_char *)dio_req->k_io_hdrp,
                                (u_char *)dio_req->u_io_hdrp,sizeof(sgat_dio_hdr_t));
        }
        dio_req->in_use = 0;
}

static void sgat_dio_done_nocopy(struct sgat_dio_fops *fops, sgat_dio_req *dio_req)
{
        if (!dio_req )
                return;	
        dio_req->in_use = 0;
        return;	
}

int sgat_dio_get_default_fops(struct sgat_dio_fops *fops)
{
        if ( !fops){
                DIO_DPRINTK("Invalid parameter\n");
                return -EINVAL;
        }
                        
        fops->sgat_dio_malloc = sgat_dio_malloc;
        fops->sgat_dio_free = sgat_dio_free;
        fops->add_dio_chunk = add_dio_chunk;
        fops->end_dio_req = end_dio_req;
        fops->sgat_dio_done = sgat_dio_done;
        fops->expand_sclist = expand_sclist;

        return 0;
}

EXPORT_SYMBOL(sgat_dio_get_default_fops);

static int compute_scbuf_size(struct iovec *iovecp,int iovec_count)
{
        struct iovec *p;
        int	i;
        int scl_entries,sg_tablesize = 0;
    
        if (!iovecp)
                return -EINVAL;	
        for (i= 0,p = iovecp;i < iovec_count ; i++,p++) {
                scl_entries = (p->iov_len / PAGE_SIZE) * 2 + 1;
                if ( 1 == scl_entries ) 
                        scl_entries = 2 ;
                sg_tablesize += scl_entries;
        }
        return sg_tablesize;
}

static int get_dxfer_len(struct iovec *iovecp,int iovec_count){
        struct iovec *p;
        int	i;
        int     total_len = 0;

        if (!iovecp)
                return -EINVAL;	

        for (i= 0,p = iovecp;i < iovec_count ; i++,p++)
                total_len += p->iov_len;
        return total_len;
}

sgat_dio_req *sgat_dio_alloc_req(struct sgat_dio_fops *fops)
{
        sgat_dio_req *dio_req;

        if ( !fops )
                return NULL;

        dio_req = (sgat_dio_req *) fops->sgat_dio_malloc(sizeof(sgat_dio_req));    
        if ( !dio_req )
                return NULL;
        return dio_req;
}
EXPORT_SYMBOL(sgat_dio_alloc_req);

int sgat_dio_init_req(struct sgat_dio_fops *fops,
                        sgat_dio_req *dio_req,
                        sgat_dio_hdr_t *u_io_hdrp)
{
        if ( !fops || !u_io_hdrp || !dio_req)
                return -EINVAL;

        dio_req->sc_elems = 0;
        dio_req->u_io_hdrp = NULL;
        dio_req->k_io_hdrp = NULL;
        dio_req->iobuf_inuse = 0;
        dio_req->k_useg = 0;

        dio_req->k_io_hdrp = (sgat_dio_hdr_t *) fops->sgat_dio_malloc(sizeof(sgat_dio_hdr_t));
        if ( !dio_req->k_io_hdrp ){
                DIO_DPRINTK("sgat_dio_malloc failed\n");
                goto errout;
        }

        if ( sgat_dio_cpfrmuser((u_char *)u_io_hdrp,
                                (u_char *)dio_req->k_io_hdrp,sizeof(sgat_dio_hdr_t)))
                        goto errout;

        dio_req->u_io_hdrp = u_io_hdrp;

        dio_req->kiobp = (struct iobuf *)sgat_dio_malloc(sizeof(struct iobuf));
        if (unlikely(!dio_req->kiobp)){
                DIO_DPRINTK("alloc_iobuf failed\n");
                goto errout;
        }

        if (unlikely(iobuf_init(dio_req->kiobp ))){
                DIO_DPRINTK("iobuf_init failed\n");
                goto errout;
        }

        dio_req->sc_elems = compute_scbuf_size(dio_req->k_io_hdrp->iovec,
                                                dio_req->k_io_hdrp->iovec_count);
        DIO_DPRINTK("sc_elems %d\n",dio_req->sc_elems);
        if (dio_req->sc_elems <= 0 ) {
                DIO_DPRINTK("Invalid sc_elems %d\n",dio_req->sc_elems);
                goto errout;
        }
        dio_req->sclist = fops->sgat_dio_malloc(dio_req->sc_elems * sizeof(struct scatterlist));
        if ( !dio_req->sclist )
                goto errout;

        return 0;
errout:
        sgat_dio_exit_req(fops, dio_req,0);
        return -ENOMEM;
}
EXPORT_SYMBOL(sgat_dio_init_req);

void sgat_dio_free_req(struct sgat_dio_fops *fops, struct sgat_dio_req *dio_req)
{
        if ( dio_req)
                fops->sgat_dio_free((void *)dio_req);
}
EXPORT_SYMBOL(sgat_dio_free_req);

void sgat_dio_exit_req(struct sgat_dio_fops *fops, struct sgat_dio_req *dio_req, int flushDcache)
{
        if ( dio_req) {
                if (dio_req->iobuf_inuse && dio_req->kiobp) {
                        unlock_pages(dio_req->kiobp->maplist);
                        if ( flushDcache){
                                flush_dcache_all();
                                //flush_dcache();
                        }
                        iobuf_release(dio_req->kiobp);
                        dio_req->iobuf_inuse = 0;
                }

                if ( dio_req->kiobp ){
                        sgat_dio_free(dio_req->kiobp);
                        dio_req->kiobp = NULL;
                }

                if ( dio_req->sclist ){
                        fops->sgat_dio_free((void *)dio_req->sclist);
                        dio_req->sclist = NULL;
                }

                if ( dio_req->k_io_hdrp ){
                        fops->sgat_dio_free((void *)dio_req->k_io_hdrp);
                        dio_req->k_io_hdrp = NULL;
                }
        }
}
EXPORT_SYMBOL(sgat_dio_exit_req);

static int sgat_dio_build_sclist(struct sgat_dio_fops *fops,
                                sgat_dio_req *dio_req)
{
        int     k,l;
        int     offset, rem_sz,useg_len,toffset;
        struct  iobuf *kp;
        void    *sclp;
        int	pgindex =0;
        int     pgcount=0;
        unsigned short iovec_count=0;
        int     res;
        int     dxfer_len;
        unsigned long p;
        unsigned long kaddress;
        int     length;
        struct iovec *iovecp ;
        unsigned short iovecc;
        int ulen;
        struct page *start_pg, *next_pg, *cur_pg;
        
        if (!fops || !dio_req || 
             !dio_req->kiobp || !dio_req->k_io_hdrp ||
             !dio_req->k_io_hdrp->iovec )
                return -EFAULT;

        kp = dio_req->kiobp; 

        iovecp = dio_req->k_io_hdrp->iovec;
        iovecc = dio_req->k_io_hdrp->iovec_count;
                        
        fops->expand_sclist(fops,dio_req, iovecc); 
        sclp = dio_req->sclist;
        if (!sclp)
                return -ENOMEM;

        dxfer_len = get_dxfer_len(iovecp, iovecc);

        for (k = 0, rem_sz = dxfer_len,iovec_count = 0,pgindex = 0;
                (iovec_count < iovecc) && (rem_sz > 0 ) && (k < iovecc);
                iovec_count++, k++) {

                p = (unsigned long) iovecp[iovec_count].iov_base;
                if ( !p )
                    return -EFAULT;
                
                useg_len = iovecp[iovec_count].iov_len;
                offset = p & (PAGE_SIZE-1);
                pgcount = (p + useg_len + PAGE_SIZE - 1)/PAGE_SIZE - p/PAGE_SIZE;
                if (!pgcount) 
                        BUG();

                if (access_ok((dio_req->k_io_hdrp->dxfer_dir == READ)?
                              VERIFY_READ:VERIFY_WRITE,
                              (u_char *)p, iovecp[iovec_count].iov_len) == 0)
                    return -EFAULT;
		
                DIO_DPRINTK ("iovec %d offset %d len"
                                "%d pgcount %d pgindex %d rem_sz %d\n",
                                iovec_count,offset,useg_len,pgcount,pgindex,rem_sz);

                for( length = 0,ulen = useg_len,l = pgindex, toffset = offset,
                        next_pg = NULL, start_pg = kp->maplist[l];
                        (ulen > 0) && (start_pg != NULL) &&
                        (l < (pgindex+pgcount)); 
                        l++ ) {

                        kaddress = (unsigned long) page_address(start_pg)+offset;

                        cur_pg = kp->maplist[l];
                        next_pg = ((l+1) < (pgindex+pgcount))? 
                                        kp->maplist[l+1]:NULL;
                        
                        if ( next_pg || (ulen / PAGE_SIZE)){
                                ulen -= PAGE_SIZE - toffset;
                                length += PAGE_SIZE - toffset;
                                toffset = 0;
                                if (next_pg && 
                                        ((unsigned long)page_address(cur_pg) + PAGE_SIZE) ==
                                        (unsigned long)page_address(next_pg))
                                        continue;
                        }
                        else {
                                /* Last page */
                                length += ulen;
                                ulen = 0;
                        }
                        sclp = fops->add_dio_chunk(fops,
                                        dio_req,
                                        kaddress, 
                                        length,sclp);
                        if ( !sclp ) 
                                return -EFAULT;
            
                        DIO_DPRINTK("k=%d, a=0x%p, len=%d"
                                        "ulen = %d\n", k,
                                        kaddress, length, ulen);
                        length = 0;
                        offset = 0;
                        toffset = 0;
                        start_pg = next_pg;
                }
                pgindex += pgcount;
                rem_sz -= useg_len;
                DIO_DPRINTK("rem_sz=%d k=%d\n",rem_sz,k);
        }
        if (rem_sz > 0)
                return -EFAULT; 

        res = fops->end_dio_req(fops,dio_req);
        if (res < 0 )
                return -EFAULT;
        
        DIO_DPRINTK("ku_seg=%d, rem_sz=%d\n",k, rem_sz);
        return 0;
}

int sgat_dio_build_sg_list(struct sgat_dio_fops *fops,
                           sgat_dio_req *dio_req, int flushDcache)
{

        int                     res;
        sgat_dio_hdr_t          *k_io_hdrp= dio_req->k_io_hdrp;
   
        if ( !dio_req || !k_io_hdrp)
	        return -EINVAL;

        if ( k_io_hdrp->iovec_count && k_io_hdrp->iovec) {
            if (access_ok(VERIFY_READ, k_io_hdrp->iovec,
                sizeof(struct iovec) * k_io_hdrp->iovec_count) == 0) {
                DIO_DPRINTK("failed access_ok\n");
                sgat_dio_free_req(fops,dio_req);
                return -EFAULT;
            }
        }
        else {
                DIO_DPRINTK("invalid inputs(iovec count %d )\n",
                        k_io_hdrp->iovec_count);
                sgat_dio_free_req(fops,dio_req);
                return -ENOMEM;
        }

        res = sgat_dio_map_user_iobuf(k_io_hdrp->dxfer_dir, dio_req->kiobp, 
                                        k_io_hdrp->iovec, k_io_hdrp->iovec_count,flushDcache);
        if (0 != res) {
                DIO_DPRINTK("map_user_iobuf res=%d\n", res);
                sgat_dio_free_req(fops,dio_req);
                return -EFAULT;
        }

        dio_req->iobuf_inuse = 1;
        res = sgat_dio_build_sclist(fops, dio_req);
        if (res < 0) {
                sgat_dio_free_req(fops,dio_req);
                return res;
        }
    
        return 0;
}
EXPORT_SYMBOL(sgat_dio_build_sg_list);

void unlock_pages(struct page **page_list)
{
        if ( !page_list )
                return;
        while (*page_list) {
                if (!PageReserved(*page_list))
                    SetPageDirty(*page_list);
                put_page(*page_list);
                page_list++;
        }
}
int pindown_pages(struct page **pg_list, 
                        struct iovec *iovec, 
                        int iovec_count, int flushDcache, int rw){
	int count,err=0;
	struct iovec *tiovec;	
        int x=0,i;
        struct page **page_list=pg_list;
        int pg_count=0;
        char *iov_base;

        /* Acquire the mm page semaphore. */
        down_read(&current->mm->mmap_sem);
	for ( count = 0, tiovec = iovec; count < iovec_count; count++, tiovec++){
                int nr_pages = (((u_long)tiovec->iov_base + 
                                tiovec->iov_len + PAGE_SIZE - 1)/PAGE_SIZE)-
                                ((u_long)tiovec->iov_base/PAGE_SIZE);
                if ( rw == READ ){
                        iov_base = (u_long) tiovec->iov_base - 
                                ((u_long) tiovec->iov_base & ~PAGE_MASK); 
                        for ( i = 0; i < nr_pages; i++, iov_base += PAGE_SIZE){
                                if ( __get_user(x, iov_base) || __put_user(x, iov_base))
                                        BUG();
                        }
                }
                err = get_user_pages(current,
                                     current->mm,
                                     (unsigned long int)tiovec->iov_base,
                                     nr_pages,
                                     rw==READ,  /* read access only for in data */
                                     0, /* no force */
                                     &page_list[pg_count],
                                     NULL);
                if (err < 0 || err < nr_pages )
                        goto err_out;
                pg_count += err;
        }
        page_list[pg_count]=NULL;
        if ( flushDcache ) {
                flush_dcache_all();
        }
err_out:	
        if (err < 0) {
                unlock_pages(pg_list);
                if (flushDcache ) {
                        flush_dcache_all();
                }
                up_read(&current->mm->mmap_sem);
                return err;
        }
	up_read(&current->mm->mmap_sem);

	return 0;
}
int sgat_dio_map_user_iobuf(int rw, struct iobuf *iobuf, 
                             struct iovec *iovec, int iovec_count, int flushDcache)
{
        int pgcount=0;
        int total_pages=0;
        int total_len=0;
	int count,err;
	struct iovec *tiovec;	

        iobuf->nr_pages = 0;
        iobuf->length  = 0;

	for ( count = 0, tiovec = iovec; count < iovec_count; count++, tiovec++){
                pgcount = (((u_long)tiovec->iov_base) + tiovec->iov_len + PAGE_SIZE - 1)/PAGE_SIZE 
                            - ((u_long)tiovec->iov_base)/PAGE_SIZE;
                total_pages += pgcount;
                total_len += tiovec->iov_len;
        }

	err = expand_iobuf(iobuf, total_pages+1);
	if (err){
		return err;
        }
        
        iobuf->nr_pages = total_pages;
        iobuf->length = total_len;

        err = pindown_pages(iobuf->maplist, iovec, iovec_count, flushDcache, rw);
        if (err < 0) {
                iobuf->nr_pages = 0;
                iobuf->length  = 0;
        }
        return err;
}
