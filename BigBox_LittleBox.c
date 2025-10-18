#include <stdio.h>
#include <stdbool.h>

char*			filename;
unsigned char*	bytStr;
enum 			SourceFormat sourceFormat;
enum 			TargetFormat targetFormat;
FILE			*source_file, *tilefile1, *tilefile2, *tilemapfile, *palfile;
long			file_size,tx,tile_x;
int				colNum,pix_loc,img_width,img_height,ty,tile_y;
short			pal_loc,img_depth,tile_depth,tile_size,coef,pix0,pix1;
bool			full_size,ref,isTileMap;

enum SourceFormat
{
 FORMAT_BMP,
 FORMAT_ROHGA_DECR,
 FORMAT_PCE_CG,
 FORMAT_PLANAR4_16x16,
 FORMAT_NEO_MIRROR,
 FORMAT_OLD_SPRITE,
 FORMAT_TAITO_Z,
 FORMAT_UNDERFIRE,
 FORMAT_HALF_DEPTH,
 FORMAT_UNKNOWN
};

enum TargetFormat
{
 TARGET_C123,
 TARGET_OLD_SPRITE,
 TARGET_MODEL3_8,
 TARGET_NEOGEO_SPR,
 TARGET_PSIKYO_LATER_GENERATIONS_8,
 TARGET_ATETRIS,
 TARGET_TC0180VCU,
 TARGET_UNKNOWN
};

struct FormatInfo
{
 const char* name;
 const char* description;
};

struct TargetInfo
{
 const char* name;
 const char* description;
};

struct ArgsInfo
{
 const char* name;
 const char* description;
};

const struct FormatInfo source_formats[] = {
    {"bmp", "standard BMP file"},
    {"rohga_decr", "decrypted 4bpp planar 8x8 tiles for Armored Force Rohga"},
    {"pce_cg", "4bpp planar 8x8 tiles for NEC/Hudson Soft PC Engine/TurboGraphX-16 basic video"},
    //{"linear4_16x16", "Generic 4bpp linear 16x16 tiles. Among the noticeable usage cases is a tiles for Konami K053246 custom sprite chip."},
    {"planar4_16x16", "Generic 4bpp planar 16x16 tiles. Among the noticeable usage cases is a sprites for Irem M92 and tiles for Taito TC0180VCU and Toaplan GP9001 custom video chips. Some later M92 games (e. g. [Superior/Perfect] Soldiers) stores the sprite data by 16px-wide rows instead of 8px-wide like other - in this case you can use the -full arg."},
    {"neo_mirror", "The closest to Neo-Geo sprites graphic format, excepting only reverse order of pixels inside each 8px-wide row. The most noticeable usage case is a sprites for some of Data East 16-bit arcades (for example, Crude Buster/Two Crude) and their various korean hardware clones."},
    {"old_sprite", "See the target formats list. Please note that in the source role its size is always gets used fully."},
    {"taito_z", "4bpp planar 16x8 sprite tiles for Taito System Z games (except Chase HQ). Some of them (e.g., Battle Shark and Space Gun) use a pre-mirrored tiles (see the -ref arg)."},
    {"underfire", "5bpp planar 16x16 sprite tiles for Taito's Under Fire hardware"},
    {"half_depth", "tricky technique of generic 4bpp linear 16x16 sprite tiles enpackagement as a sprite data for Namco C355"},
    {NULL, NULL}
};

const struct TargetInfo target_formats[] = {
    {"c123", "8bpp linear 8x8 tiles for Namco C123 (identical to GBA and SNES Mode7 tiles)"},
    {"old_sprite", "8bpp planar 32x32 tiles for early Namco System 2 versions sprite subsystem. Besides the full hardware tile space, converter user can also occupy its upper left quarter only (see the -full arg), which can be helpful for a smaller sprites usage in the homebrew games and hacks for NS2."},
    {"model3_8", "8bpp linear 8x8 tiles for Sega Model 3 tilemaps"},
    {"neogeo_spr", "4bpp planar 16x16 sprites for Neo-Geo MVS\\AES"},
    {"psikyo_later_generations_8", "8bpp linear 16x16 tiles for Psikyo's SH-2 based arcade machines"},
    {"atetris", "4bpp linear 8x8 tiles for Atari's Tetris arcade hardware (identical to Sega Genesis/Mega Drive and MSX tiles)"},
    {"tc0180vcu", "4bpp planar tiles for Taito TC0180VCU custom video chip, used mainly by Taito System B arcade platform. Can be output both in 8x8 and 16x16 (using a -full arg) form. Generated palette data is 12-bit RGBx."},
    {NULL, NULL}
};

const struct ArgsInfo additional_args[] = {
    {"tm", "generate tilemap (only for BMP images and non-8x8 tile formats)"},
    {"full", "use a larger version of some tile formats (only for planar4_16x16 source and old_sprite and tc0180vcu targets)"},
    {"ref", "Reflect an input or output (depends on the source and target formats combination) tiles. This feature is used by taito_z (horizontal) and tc0180vcu (vertical, as a target exclusively) only."},
    {"h, --help", "show this help message"},
    {NULL, NULL}
};

enum SourceFormat GetSourceFormat(const char* arg)
{
 if(strcmp(arg,"bmp")==0)							return FORMAT_BMP;
 if(strcmp(arg,"rohga_decr")==0)					return FORMAT_ROHGA_DECR;
 if(strcmp(arg,"pce_cg")==0)						return FORMAT_PCE_CG;
 if(strcmp(arg,"planar4_16x16")==0)					return FORMAT_PLANAR4_16x16;
 if(strcmp(arg,"neo_mirror")==0)					return FORMAT_NEO_MIRROR;
 if(strcmp(arg,"old_sprite")==0)					return FORMAT_OLD_SPRITE;
 if(strcmp(arg,"taito_z")==0)						return FORMAT_TAITO_Z;
 if(strcmp(arg,"underfire")==0)						return FORMAT_UNDERFIRE;
 if(strcmp(arg,"half_depth")==0)					return FORMAT_HALF_DEPTH;

 return FORMAT_UNKNOWN;
}

enum TargetFormat GetTargetFormat(const char* arg)
{
 if(strcmp(arg,"c123")==0)							return TARGET_C123;
 if(strcmp(arg,"old_sprite")==0)					return TARGET_OLD_SPRITE;
 if(strcmp(arg,"model3_8")==0)						return TARGET_MODEL3_8;
 if(strcmp(arg,"neogeo_spr")==0)					return TARGET_NEOGEO_SPR;
 if(strcmp(arg,"psikyo_later_generations_8")==0)	return TARGET_PSIKYO_LATER_GENERATIONS_8;
 if(strcmp(arg,"atetris")==0)						return TARGET_ATETRIS;
 if(strcmp(arg,"tc0180vcu")==0)						return TARGET_TC0180VCU;

 return TARGET_UNKNOWN;
}

void print_help(const char* program_name)
{
 int i;

 printf("Multi-format tile data conversion tool.\n");
 printf("Usage: %s <srcfile> -in [source_format] -out [target_format] [options]\n\n",program_name);
 printf("Options:\n");

 for(i=0;additional_args[i].name!=NULL;i++) printf("  -%-29s %s\n",additional_args[i].name,additional_args[i].description);

 printf("\nSource formats available:\n");

 for(i=0;source_formats[i].name!=NULL;i++) printf("  %-30s %s\n",source_formats[i].name,source_formats[i].description);

 printf("  (default: bmp)\n");

 printf("\nTarget formats available:\n");

 for(i=0;target_formats[i].name!=NULL;i++) printf("  %-30s %s\n",target_formats[i].name,target_formats[i].description);

 printf("  (default: c123)\n");
}

void print_arguments(int argc,char* argv[])
{
 int i;

 printf("Command line arguments:\n");

 for(i=0;i<argc;i++)
 {
  printf("argv[%d] = %s\n",i,argv[i]);
 }
}

void process_arguments(int argc,char* argv[])
{
 int i;

 //Default values
 sourceFormat=FORMAT_BMP;
 targetFormat=TARGET_C123;
 isTileMap=false;

 for(i=1;i<argc;i++)
 {
  if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0)
  {
   print_help(argv[0]);
   exit(0);
  }
  else if(strcmp(argv[i],"-in")==0&&i+1<argc)
  {
   sourceFormat=GetSourceFormat(argv[++i]);
   if(sourceFormat==FORMAT_UNKNOWN)
   {
    printf("Unknown input format: %s\n",argv[i]);
    exit(1);
   }
  }
  else if(strcmp(argv[i],"-out")==0&&i+1<argc)
  {
   targetFormat=GetTargetFormat(argv[++i]);
   if(targetFormat==TARGET_UNKNOWN)
   {
    printf("Unknown output format: %s\n",argv[i]);
    exit(1);
   }
  }
  else if(strcmp(argv[i],"-tm")==0)		isTileMap=true;
  else if(strcmp(argv[i],"-full")==0)	full_size=true;
  else if(strcmp(argv[i],"-ref")==0)	ref=true;
  else if(i==1)							filename=argv[i];
  else
  {
   printf("Unknown argument: %s\n",argv[i]);
   print_help(argv[0]);
   exit(1);
  }
 }
}

void check_format()
{
 const char *extension;
 char *dot=strrchr(filename,'.');

 switch(sourceFormat)
 {
  case FORMAT_BMP:
  	   extension=".bmp";
  	   if(bytStr[0]!='B'||bytStr[1]!='M')
  	   {
       	printf("BMP file signature isn't found!\n");
       	fclose(source_file);
  	   	exit(1);
	   }

	   if((bytStr[2]|(bytStr[3]<<8)|(bytStr[4]<<16)|(bytStr[5]<<24))!=file_size)
	   {
       	printf("The file is broken!\n");
       	fclose(source_file);
  	   	exit(1);
	   }

	   if(bytStr[30]|(bytStr[31]<<8)|(bytStr[32]<<16)|(bytStr[33]<<24)!=0)
	   {
       	printf("Compressed images is not supported for a while.\n");
       	fclose(source_file);
  	   	exit(1);
	   }

   	   img_width=(bytStr[18]|(bytStr[19]<<8)|(bytStr[20]<<16)|(bytStr[21]<<24));
   	   img_height=(bytStr[22]|(bytStr[23]<<8)|(bytStr[24]<<16)|(bytStr[25]<<24));
   	   img_depth=bytStr[28];
   	   pal_loc=54;
   	   
   	   //if(img_depth==4) pix_loc=118;
   	   //else if(img_depth==8) pix_loc=1078;

	   if(img_width%tile_size!=0||img_height%tile_size!=0)
	   {
       	printf("At least the one of image size parameters is not power-of-%d!\n",tile_size);
       	fclose(source_file);
  	   	exit(1);
	   }
   	   
  	   break;
  default:
       printf("Unsupported source format\n");
       fclose(source_file);
  	   exit(1);
 }
  
 if(dot==NULL||(strcasecmp(dot,extension)!=0))
 {
  printf("Loaded file extension doesn't match!\n");
  fclose(source_file);
  exit(1);
 }

 if(img_depth<=8) coef=8/img_depth;
}

int get_tile_el_value(short x,short y)
{
 if(sourceFormat==FORMAT_BMP)					return bytStr[file_size-(((tile_y*tile_size+y)*img_width)/coef+((img_width-tile_x*tile_size)/coef-x))];
 else if(sourceFormat==FORMAT_ROHGA_DECR)		return bytStr[(file_size/2)*((x%4)/2)+tile_x*16+(x%2)+y*2];
 else if(sourceFormat==FORMAT_PCE_CG)			return bytStr[16*((x%4)/2)+tile_x*32+(x%2)+y*2];
 else if(sourceFormat==FORMAT_PLANAR4_16x16)	return bytStr[(targetFormat==TARGET_NEOGEO_SPR?tile_x:tile_x/4)*128+(full_size==true?(x%2):(((targetFormat==TARGET_NEOGEO_SPR?x:tile_x)%2)*64+(((targetFormat==TARGET_NEOGEO_SPR?x:tile_x)%4)/2)*32))+(targetFormat==TARGET_NEOGEO_SPR?(x/2)*2:x)+((y*4)<<full_size)];
 else if(sourceFormat==FORMAT_NEO_MIRROR)		return bytStr[tile_x*128+((x/4)%2)*64+(x%4)+(y*4)];
 else if(sourceFormat==FORMAT_OLD_SPRITE)		return bytStr[(tile_x/16)*1024+(tile_x%4)*8+((tile_x%16)/4)*256+(x/4)*4+x/2+y*32];
 else if(sourceFormat==FORMAT_TAITO_Z)			return bytStr[(targetFormat==TARGET_NEOGEO_SPR?tile_x:tile_x/2)*64+(ref==true?1-((targetFormat==TARGET_NEOGEO_SPR?x:tile_x)%2):(targetFormat==TARGET_NEOGEO_SPR?x:tile_x)%2)+(3-x)*2+y*8];
 else if(sourceFormat==FORMAT_UNDERFIRE)		return bytStr[(tile_x/4)*160+(1-(tile_x%2))*5+((tile_x%4)/2)*80+x+y*10];
 else if(sourceFormat==FORMAT_HALF_DEPTH)		return bytStr[((tile_x%(file_size/tile_size*tile_depth/2))/4)*256+(tile_x%2)*8+((tile_x%4)/2)*128+x+y*16];

 return 0;
}

bool tiles_match(long prev_tx,int prev_ty)
{
 short x,y;

 //Check against previous tiles for duplicates
 for(y=0;y<tile_size;y++)
 {
  for(x=0;x<(sourceFormat<FORMAT_ROHGA_DECR?tile_size/coef:tile_depth);x++)
  {
   tile_x=tx;
   tile_y=ty;

   short tile_el1=get_tile_el_value(x,y);

   tile_x=prev_tx;
   tile_y=prev_ty;

   short tile_el2=get_tile_el_value(x,y);

   if(tile_el1!=tile_el2) return false;
  }
 }
 return true;
}

//Standart colour spaces
void rgb888()
{
 for(colNum=0;colNum<(1<<img_depth);colNum++)
 {
  fputc(bytStr[pal_loc+colNum*4+2],palfile);
  fputc(bytStr[pal_loc+colNum*4+1],palfile);
  fputc(bytStr[pal_loc+colNum*4],palfile);
 }
}

void rgb332()
{
 for(colNum=0;colNum<(1<<img_depth);colNum++) fputc((((bytStr[pal_loc+colNum*4+2]>>5)<<5)|((bytStr[pal_loc+colNum*4+1]>>5)<<2)|(bytStr[pal_loc+colNum*4]>>6)),palfile);
}

//Specific colour spaces
void model3_tilemap_pal()
{
 for(colNum=0;colNum<(1<<img_depth);colNum++)
 {
  fputc((((bytStr[pal_loc+colNum*4+1]>>3)<<10)|((bytStr[pal_loc+colNum*4]>>3)<<5)|(bytStr[pal_loc+colNum*4+2]>>3))&0xFF,palfile);
  fputc((((bytStr[pal_loc+colNum*4+1]>>3)<<10)|((bytStr[pal_loc+colNum*4]>>3)<<5)|(bytStr[pal_loc+colNum*4+2]>>3))>>8,palfile);
  fputc(0,palfile);
  fputc(0,palfile);
 }
}

void rgb444x()
{
 for(colNum=0;colNum<(1<<img_depth);colNum++)
 {
  fputc(bytStr[pal_loc+colNum*4+2]&0xf0|(bytStr[pal_loc+colNum*4+1]>>4),palfile);
  fputc(bytStr[pal_loc+colNum*4]&0xf0,palfile);
 }
}

int main(int argc,char *argv[])
{
#ifdef _DEBUG
 print_arguments(argc,argv);
#endif //

 if(argc<2)
 {
  print_help(argv[0]);
  return 1;
 }

 process_arguments(argc,argv);

 if((source_file=fopen(filename,"rb"))==NULL)
 {
  printf("Can't open input file\n");
  return 1;
 }


/*
 *	Now we try to determine the size of the file
 *	to be converted
 */
 if(fseek(source_file,0,SEEK_END))
 {
  printf("Couldn't determine size of file\n");
  fclose(source_file);
  return 1;
 }

 file_size=ftell(source_file);

 char* dot_pos=strrchr(filename,'.');
 char tilename1[256];
 char tilename2[256];
 char tmap_name[256];
 char palname[256];
 if(dot_pos!=NULL)
 {
  size_t name_lenght=dot_pos-filename;
  char extless[name_lenght+1];
  strncpy(extless,filename,name_lenght);
  extless[name_lenght]='\0';
  if(targetFormat==TARGET_NEOGEO_SPR)
  {
   snprintf(tilename1,sizeof(tilename1),"%s.c1",extless);
   snprintf(tilename2,sizeof(tilename2),"%s.c2",extless);
  }
  else							snprintf(tilename1,sizeof(tilename2),"%s.bin",extless);

  if(isTileMap)					snprintf(tmap_name,sizeof(tmap_name),"%s_tilemap.bin",extless);
  if(sourceFormat==FORMAT_BMP)	snprintf(palname,sizeof(palname),"%s_pal.bin",extless);
 }
 else
 {
  if(targetFormat==TARGET_NEOGEO_SPR)
  {
   snprintf(tilename1,sizeof(tilename1),"%s.c1",filename);
   snprintf(tilename2,sizeof(tilename2),"%s.c2",filename);
  }
  else							snprintf(tilename1,sizeof(tilename1),"%s.bin",filename);

  if(isTileMap)					snprintf(tmap_name,sizeof(tmap_name),"%s_tilemap.bin",filename);
  if(sourceFormat==FORMAT_BMP)	snprintf(palname,sizeof(palname),"%s_pal.bin",filename);
 }

 if((tilefile1=fopen(tilename1,"wb"))==NULL)
 {
  printf("Can't open output file\n");
  fclose(source_file);
  return 1;
 }

 if(targetFormat==TARGET_NEOGEO_SPR)
 {
  if((tilefile2=fopen(tilename2,"wb"))==NULL)
  {
   printf("Can't open output file\n");
   fclose(source_file);
   return 1;
  }
 }

 if(isTileMap)
 {
  if((tilemapfile=fopen(tmap_name,"wb"))==NULL)
  {
   printf("Can't open tilemap file\n");
   fclose(source_file);
   return 1;
  }
 }

 if(sourceFormat==FORMAT_BMP)
 {
  if((palfile=fopen(palname,"wb"))==NULL)
  {
   printf("Can't open output file\n");
   fclose(source_file);
   return 1;
  }
 }

 fseek(source_file,0L,SEEK_SET);

 bytStr=(unsigned char*)malloc(file_size);
 if(bytStr==NULL)
 {
  printf("Memory allocation failed (at the format check stage)\n");
  fclose(source_file);
  exit(1);
 }

 fread(bytStr,1,file_size,source_file);

 //Process based on target format
 if(full_size==true&&!(sourceFormat==FORMAT_PLANAR4_16x16||targetFormat==TARGET_OLD_SPRITE||targetFormat==TARGET_TC0180VCU))
 {
  printf("Selected source or target format has only one variation of size.\n");
  fclose(source_file);
  exit(1);
 }

 if(ref==true&&!(sourceFormat==FORMAT_TAITO_Z||targetFormat==TARGET_TC0180VCU))
 {
  printf("Neither target nor source format use a horizontal or vertical reflection.\n");
  fclose(source_file);
  exit(1);
 }

 short depth;

 //Get target format tile size
 if(targetFormat==TARGET_OLD_SPRITE)															tile_size=16<<full_size;
 else if(targetFormat==TARGET_NEOGEO_SPR||targetFormat==TARGET_PSIKYO_LATER_GENERATIONS_8)		tile_size=16;
 else if(targetFormat==TARGET_TC0180VCU)														tile_size=8<<full_size;
 else																							tile_size=8;

 //Then depth
 if(targetFormat==TARGET_NEOGEO_SPR||targetFormat>TARGET_PSIKYO_LATER_GENERATIONS_8)			depth=4;
 else																							depth=8;

 long	tiles_x;
 short	tiles_y;

 if((targetFormat==TARGET_MODEL3_8&&!(sourceFormat<=FORMAT_ROHGA_DECR||sourceFormat==FORMAT_PCE_CG||(sourceFormat==FORMAT_PLANAR4_16x16&&full_size==false)||sourceFormat==FORMAT_OLD_SPRITE||sourceFormat==FORMAT_TAITO_Z||sourceFormat==FORMAT_UNDERFIRE||sourceFormat==FORMAT_HALF_DEPTH))
	 ||(targetFormat==TARGET_NEOGEO_SPR&&!(sourceFormat==FORMAT_PLANAR4_16x16||sourceFormat==FORMAT_NEO_MIRROR||sourceFormat==FORMAT_TAITO_Z))
	 ||!(targetFormat==TARGET_NEOGEO_SPR||targetFormat==TARGET_MODEL3_8)&&sourceFormat>=FORMAT_ROHGA_DECR)
 {
  printf("This source and target formats combination doesn't supported!\n");
  fclose(source_file);
  exit(1);
 }

 if(isTileMap&&(targetFormat==TARGET_OLD_SPRITE||targetFormat==TARGET_NEOGEO_SPR))
 {
  printf("Chosen target format is a sprites, not a tilemaps.\n");
  fclose(source_file);
  exit(1);
 }

 if(sourceFormat<FORMAT_ROHGA_DECR)
 {
  check_format();

  if(img_depth>8)
  {
   printf("Full- and true-coloured images import isn't supported for a while.\n");
   fclose(source_file);
   exit(1);
  }

  if(img_depth!=depth)
  {
   printf("Chosen format uses a %d-bit pixels.\n",depth);
   fclose(source_file);
   exit(1);
  }

  tiles_x=img_width/tile_size;
  tiles_y=img_height/tile_size;
 }
 else
 {
  if((sourceFormat==FORMAT_ROHGA_DECR||sourceFormat==FORMAT_PCE_CG)&&isTileMap==true)
  {
   printf("8x8 tiles formats doesn't need an extra optimization.\n");
   fclose(source_file);
   exit(1);
  }
  	 
  if(sourceFormat==FORMAT_OLD_SPRITE)		tile_depth=tile_size; //single pixel accords a single byte
  else if(sourceFormat==FORMAT_UNDERFIRE)	tile_depth=5;
  else										tile_depth=4;
  tiles_x=file_size/((tile_size*(sourceFormat==FORMAT_TAITO_Z?8:tile_size)*tile_depth)/8);
  tiles_y=1; //Because a tile data, unlike the standart GFX files, hasn't a size parameters by themselves, it'd be a more expedient to present all the data piece as a very-very long tiles row
 }

 //Process tiles
 long	unique_tiles_base[tiles_x*tiles_y],tile_index,prev_tx,matched_tx,unique_tiles=0;
 int	prev_ty,matched_ty;
 short	tiles[sourceFormat<FORMAT_ROHGA_DECR?img_width*img_height:tile_size*tile_size*depth/8],x,y,z;
 bool	match_found;

 for(ty=0;ty<tiles_y;ty++)
 {
  for(tx=0;tx<tiles_x;tx++)
  {
   match_found=false;
   if(isTileMap)
   {
   	//Check against previous tiles for duplicates
	for(prev_ty=0;prev_ty<=ty&&!match_found;prev_ty++)
	{
	 for(prev_tx=0;prev_tx<(prev_ty==ty?tx:tiles_x)&&!match_found;prev_tx++)
	 {
	  if(tiles_match(prev_tx,prev_ty))
	  {
	   match_found=true;
	   matched_tx=prev_tx;
	   matched_ty=prev_ty;
	  }
	 }
    }

	if(match_found)
	{
	 for(tile_index=0;tile_index<unique_tiles;tile_index++)
	 {
	  if(unique_tiles_base[tile_index]==matched_ty*tiles_x+matched_tx) break;
     }
	 fputc((tile_index>>24)&0xff,tilemapfile);
	 fputc((tile_index>>16)&0xff,tilemapfile);
	 fputc((tile_index>>8)&0xff,tilemapfile);
	 fputc(tile_index&0xff,tilemapfile);
    }
	else
	{
	 if(tx>0||ty>0)	unique_tiles++;
	 unique_tiles_base[unique_tiles]=ty*tiles_x+tx;
	 fputc((unique_tiles>>24)&0xff,tilemapfile);
	 fputc((unique_tiles>>16)&0xff,tilemapfile);
	 fputc((unique_tiles>>8)&0xff,tilemapfile);
	 fputc(unique_tiles&0xff,tilemapfile);
    }
   }

   if(!match_found)
   {
   	tile_x=tx;
	tile_y=ty;

	for(y=0;y<tile_size;y++)
 	{
	 if(sourceFormat<FORMAT_ROHGA_DECR)
	 {
	  if(targetFormat==TARGET_TC0180VCU)
	  {
	   for(z=0;z<img_depth;z++)
	   {
	   	for(x=0;x<tile_size/coef;x++)
	   	{
	   	 pix0|=((get_tile_el_value(x,(ref==true?tile_size-y:y))>>4)&(1<<z)>>z)<<(7-(x%8)*2);
		 pix1|=((get_tile_el_value(x,(ref==true?tile_size-y:y))&0xf)&(1<<z)>>z)<<(7-(x%8)*2+1);

		 if(full_size==true&&x==tile_size/coef/2)
		 {
		  fputc(pix0,tilefile1);
		  fputc(pix1,tilefile1);
		  pix0=pix1=0;
         }
        }
	   	fputc(pix0,tilefile1);
	   	fputc(pix1,tilefile1);
	   	pix0=pix1=0;
       }
      }
	  else
	  {
	   if(targetFormat==TARGET_C123||targetFormat==TARGET_PSIKYO_LATER_GENERATIONS_8||targetFormat==TARGET_ATETRIS)
	   {
	   	for(x=0;x<tile_size/coef;x++) fputc(get_tile_el_value(x,y),tilefile1);
       }
	   else
	   {
	   	for(x=0;x<tile_size;x++)
	   	{
		 if(targetFormat==TARGET_OLD_SPRITE)
		 {
	   	  for(z=0;z<8;z++) tiles[(tile_y*tiles_y+tile_x)*1024+y*32+(x/4)*4+(7-z)/2]|=((((get_tile_el_value(x,y)&(1<<(z%8))))>>(z%8))<<(3-(x%4)))<<((1-(z%2))*4);
		 }
		 else fputc(get_tile_el_value(x/4+(3-(x%4)),y),tilefile1);
        }
       }
      }
	 }
	 else
	 {
      for(x=0;x<tile_size;x++)
      {
       if(sourceFormat==FORMAT_HALF_DEPTH)
       {
        pix0=get_tile_el_value(z,y);
        if(tx<tiles_x/2)	pix0>>4;
        else				pix0&=0xf;
       }
       for(z=0;z<tile_depth;z++)
       {
        if(targetFormat==TARGET_NEOGEO_SPR) tiles[tiles_x*128+(x/8)*64+y*16+z]|=(((get_tile_el_value((x/8)*4+z,y))&(1<<(7-(x%8))))>>(7-(x%8)))<<(x%8);
        else
        {
         pix1=get_tile_el_value(z,y);
         if(sourceFormat==FORMAT_OLD_SPRITE)
         {
          if(z%2)	pix1>>4;
          else		pix1&=0xf;

          pix1=(((pix1&(1<<(z%4)))>>(z%4))<<(7-z));
         }
         else	pix1=((pix1&(1<<(((7-x)/4)*4+(x%4))))>>(((7-x)/4)*4+(x%4)))<<z;
        }
       }

       if(targetFormat==TARGET_MODEL3_8)
	   {
        if(sourceFormat!=FORMAT_HALF_DEPTH)	pix0|=pix1;
		fputc(pix0,tilefile1);
        if(sourceFormat!=FORMAT_HALF_DEPTH)	pix0=0;
	   }
      }
	 }
    }
   }
  }
 }
 if(targetFormat==TARGET_OLD_SPRITE||targetFormat==TARGET_NEOGEO_SPR)
 {
  long i;
  for(i=0;i<sourceFormat<FORMAT_ROHGA_DECR?img_width*img_height:tile_size*tile_size*depth/8;i++)
  {
   if(targetFormat!=TARGET_NEOGEO_SPR||(targetFormat==TARGET_NEOGEO_SPR&&i%4<2))	fputc(tiles[i],tilefile1);
   else if(targetFormat==TARGET_NEOGEO_SPR&&i%4>=2)									fputc(tiles[i],tilefile2);
  }
 }

 if(sourceFormat<FORMAT_ROHGA_DECR)
 {
  switch (targetFormat)
  {
   case TARGET_MODEL3_8:
   		model3_tilemap_pal();
		break;
   case TARGET_NEOGEO_SPR:
		break;
   case TARGET_ATETRIS:
   		rgb332();
		break;
   case TARGET_TC0180VCU:
   		rgb444x();
		break;
   default:
        rgb888();
		break;
  }
 }

 fclose(source_file);
 fclose(tilefile1);
 if(targetFormat==TARGET_NEOGEO_SPR)	fclose(tilefile2);
 if(isTileMap)							fclose(tilemapfile);
 if(palfile)							fclose(palfile);
}
