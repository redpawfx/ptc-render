//////////////////////////////////////////////////////////////////////////

#ifdef LINUX
#define DLLEXPORT
#define SIZEOF_VOID_P 8
#else
#define DLLEXPORT __declspec(dllexport)
#endif
#define MAKING_DSO

#define SESI_LITTLE_ENDIAN 1

// CRT
#include <iostream>
using namespace std;

// H
#include <UT/UT_BoundingBox.h>
#include <UT/UT_DeepString.h>
#include <VRAY/VRAY_Procedural.h>

#include <GEO/GEO_Point.h>

#include <GU/GU_Detail.h>

// CHILD PROCEDURAL

class vray_ChildPtcBox : public VRAY_Procedural
{
private:
	UT_BoundingBox m_bb;
	UT_DeepString m_file;
	float m_tune;

public:
	vray_ChildPtcBox(float*,UT_String,float);
	virtual ~vray_ChildPtcBox() {};

	virtual const char	*getClassName()	{ return "vray_ChildPtcBox"; }

	virtual int		 initialize(const UT_BoundingBox *) { return 0; };

	virtual void getBoundingBox(UT_BoundingBox &box) { box = m_bb; };

	virtual void	 render();
};

void vray_ChildPtcBox::render()
	{
		float one = 1.0f;
		// ...SETUP DETAIL
		GU_Detail	*gdp = allocateGeometry();
		GB_AttributeRef wr = gdp->addPointAttrib("width",4,GB_ATTRIB_FLOAT,&one);
		GB_AttributeRef cdr = gdp->addDiffuseAttribute(GEO_POINT_DICT);

		// BUILD
		FILE* f = fopen(m_file.buffer(),"rb");

#define PACKETSIZE 1024

		float d[8*PACKETSIZE];

		int total = 0;

		do 
		{
			int r = fread(&d,32,PACKETSIZE,f);
			total+=r;
			float* p = d;

			for(int i=0;i<r;i++)
			{
				GEO_Point* pt = gdp->appendPoint();
				pt->setPos(p[0],p[1],p[2]);

				float* w = pt->castAttribData<float>(wr);
				*w = m_tune*p[3];

				float* cd = pt->castAttribData<float>(cdr);
				memcpy(cd,p+4,12);

				p += 8;
			};

		}
		while (!feof(f));

		fclose(f);

		cout << "File " << m_file.buffer() << " produced " << total << " points" << endl;

		// ADD GEOMETRY
		openGeometryObject();
		addGeometry(gdp, 0);
		closeObject();
};

vray_ChildPtcBox::vray_ChildPtcBox(float* b, UT_String name, float tune)
	: m_bb(b[0],b[2],b[4],b[1],b[3],b[5])
	, m_file(name)
	, m_tune(tune)
{
	int i=0;
};

// MAIN PROCEDURAL

class VRAY_Ptc : public VRAY_Procedural
{
public:
	VRAY_Ptc();
	virtual ~VRAY_Ptc() {};

	virtual const char* getClassName();
	virtual int		 initialize(const UT_BoundingBox *);
	virtual void	 getBoundingBox(UT_BoundingBox &box);
	virtual void	 render();

private:
	UT_BoundingBox	 m_bbox;		// Bounding box

	UT_String m_atlas;
	UT_String m_clause;
	float m_tune;
};

VRAY_Ptc::VRAY_Ptc()
	: m_tune(1.0f)
{
	cout << "Init...";
	m_bbox.initBounds(0,0,0);
	cout << "...done" << endl;
};

const char* VRAY_Ptc::getClassName() { return "VRAY_Ptc"; };

int VRAY_Ptc::initialize(const UT_BoundingBox *)
{
	 import("atlas", m_atlas);
	 import("clause", m_clause);

	 import("tune", &m_tune, 1);

	 FILE* fp = fopen(m_atlas,"rb");

	 int cnt;
	 fread(&cnt,sizeof(int),1,fp);

	 float bb[6];

	 if(fread(bb,sizeof(float),6,fp)==6)
	 {
		 m_bbox = UT_BoundingBox(bb[0],bb[2],bb[4],bb[1],bb[3],bb[5]);
	 };

	 fclose(fp);

	return 1;
}

void VRAY_Ptc::getBoundingBox(UT_BoundingBox &box)
{
	// Initialize with the bounding box of the stamp procedural
	box = m_bbox;

	// Each child box can also enlarge the bounds by the size of the child ?
	//box.enlargeBounds(0, mySize);
}

void VRAY_Ptc::render()
{
	FILE* fp = fopen(m_atlas.buffer(),"rb");

	int cnt;
	fread(&cnt,sizeof(int),1,fp);

	float bb[6];
	fread(bb,sizeof(float),6,fp);

	int m[3];
	fread(m,sizeof(int),3,fp);
 
	//	Create a new procedural object

	for(int i=0;i<cnt;i++)
	{
		// BBOX
		float b[6];
		fread(b,sizeof(float),6,fp);

		// CAT
		int cat[3];
		fread(cat,sizeof(int),3,fp);

		// FILE
		char nmbuff[1024];
		sprintf(nmbuff,"%s.%04i.ptc",m_clause.buffer(),i);

		UT_String name = nmbuff;

		openProceduralObject();
		addProcedural(new vray_ChildPtcBox(b,name,m_tune));
		closeObject();
	};

	fclose(fp);
}

// ENTRY POINT

VRAY_Procedural* allocProcedural(const char *)
{
	cout << "Allocate...";
	//bool va = true;
	//do 
	//{
	//	int k=0;
	//}
	//while (va);
	return new VRAY_Ptc();
	cout << "...done" << endl;
};


// Arguments for the procedural
static VRAY_ProceduralArg	theArgs[] = {
	VRAY_ProceduralArg("atlas","string",""),
	VRAY_ProceduralArg("clause","string",""),
	VRAY_ProceduralArg("tune","real","1.0"),
	VRAY_ProceduralArg()
};

const VRAY_ProceduralArg *getProceduralArgs(const char *)
{
	return theArgs; 
};