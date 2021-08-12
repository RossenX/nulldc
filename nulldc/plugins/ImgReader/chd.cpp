#include "chd.h"
#include "deps/chd.h"

struct CHDDisc : Disc
{
	chd_file* chd;
	u8* hunk_mem;
	u32 old_hunk;

	u32 hunkbytes;
	u32 sph;
	
	CHDDisc()
	{
		chd=0;
		hunk_mem=0;
	}

	bool TryOpen(wchar* file);

	~CHDDisc() 
	{ 
		if (hunk_mem)
			delete [] hunk_mem;
		if (chd)
			chd_close(chd);
	}
};

struct CHDTrack:TrackFile
{
	CHDDisc* disc;
	u32 StartFAD;
	u32 StartHunk;

	CHDTrack(CHDDisc* disc, u32 StartFAD,u32 StartHunk) 
	{ 
		this->disc=disc; 
		this->StartFAD=StartFAD;
		this->StartHunk=StartHunk;
	}

	virtual void Read(u32 FAD,u8* dst,SectorFormat* sector_type,u8* subcode,SubcodeFormat* subcode_type)
	{
		u32 fad_offs=FAD-StartFAD;
		u32 hunk=(fad_offs)/disc->sph + StartHunk;
		if (disc->old_hunk!=hunk)
		{
			chd_read(disc->chd,hunk,disc->hunk_mem); //CHDERR_NONE
		}

		u32 hunk_ofs=fad_offs%disc->sph;

		memcpy(dst,disc->hunk_mem+hunk_ofs*(2352+96),2352);
		*sector_type=SECFMT_2352;
		
		//While space is reserved for it, the images contain no actual subcodes
		//memcpy(subcode,disc->hunk_mem+hunk_ofs*(2352+96)+2352,96);
		*subcode_type=SUBFMT_NONE;
	}
};

bool CHDDisc::TryOpen(wchar* file)
{
	chd_error err=chd_open(file,CHD_OPEN_READ,0,&chd);

	if (err!=CHDERR_NONE)
		return false;

	const chd_header* head = chd_get_header(chd);

	hunkbytes = head->hunkbytes;
	hunk_mem = new u8[hunkbytes];
	old_hunk=0xFFFFFFF;

	sph = hunkbytes/(2352+96);

	if (hunkbytes%(2352+96)!=0) 
		return false;
	
	u32 tag;
	u8 flags;
	char temp[512];
	u32 temp_len;
	u32 total_frames=150;

	u32 total_secs=0;
	u32 total_hunks=0;

	for(;;)
	{
		char type[64],subtype[32]="NONE",pgtype[32],pgsub[32];
		int tkid,frames,pregap=0,postgap=0;
		err=chd_get_metadata(chd,CDROM_TRACK_METADATA2_TAG,tracks.size(),temp,sizeof(temp),&temp_len,&tag,&flags);
		if (err==CHDERR_NONE)
			//CDROM_TRACK_METADATA2_TAG 	"TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d PREGAP:%d PGTYPE:%s PGSUB:%s POSTGAP:%d"
			sscanf(temp,CDROM_TRACK_METADATA2_FORMAT,&tkid,type,subtype,&frames,&pregap,pgtype,pgsub,&postgap);
		else if (CHDERR_NONE== (err=chd_get_metadata(chd,CDROM_TRACK_METADATA_TAG,tracks.size(),temp,sizeof(temp),&temp_len,&tag,&flags)) )
			//CDROM_TRACK_METADATA_FORMAT	"TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d"
			sscanf(temp,CDROM_TRACK_METADATA_FORMAT,&tkid,type,subtype,&frames);
		else
			break;


		if (tkid!=(tracks.size()+1) || (strcmp(type,"MODE1_RAW")!=0 && strcmp(type,"AUDIO")!=0) || strcmp(subtype,"NONE")!=0 || pregap!=0 || postgap!=0)
			return false;
		printf("%s\n",temp);
		Track t;
		t.StartFAD=total_frames;
		total_frames+=frames;
		t.EndFAD=total_frames-1;
		t.ADDR=0;
		t.CTRL=strcmp(type,"AUDIO")==0?0:4;
		t.file = new CHDTrack(this,t.StartFAD,total_hunks);

		total_hunks+=frames/sph;
		if (frames%sph)
			total_hunks++;

		tracks.push_back(t);
	}

	if (total_frames!=549300 || tracks.size()<3)
		return false;

	FillGDSession();

	return true;
}


Disc* chd_parse(wchar* file)
{
	CHDDisc* rv = new CHDDisc();
	
	if (rv->TryOpen(file))
		return rv;
	else
	{
		delete rv;
		return 0;
	}
}