#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>
#include <TH1.h>
#include <TH2.h>
#include <time.h>
#include <TStopwatch.h>
#include <TFile.h>
#include <TROOT.h>
#include <TNtuple.h>


TH1F *h301 = new TH1F("h301", "X Dev 2  1234 ", 51, -1.02, 1.02);
TH1F *h302 = new TH1F("h302", "X Dev 3  1234 ", 51, -1.02, 1.02);

TH1F *h305 = new TH1F("h305", "X Dev 2  123 ", 51, -1.02, 1.02);
TH1F *h306 = new TH1F("h306", "X Dev 2  124 ", 51, -1.02, 1.02);
TH1F *h307 = new TH1F("h307", "X Dev 3  134 ", 51, -1.02, 1.02);
TH1F *h308 = new TH1F("h308", "X Dev 3  234 ", 51, -1.02, 1.02);

TH1F *h311 = new TH1F("h311", "Y Dev 2  1234 ", 51, -1.02, 1.02);
TH1F *h312 = new TH1F("h312", "Y Dev 3  1234 ", 51, -1.02, 1.02);
TH1F *h315 = new TH1F("h315", "Y Dev 2  123 ", 51, -1.02, 1.02);
TH1F *h316 = new TH1F("h316", "Y Dev 2  124 ", 51, -1.02, 1.02);
TH1F *h317 = new TH1F("h317", "Y Dev 3  134 ", 51, -1.02, 1.02);
TH1F *h318 = new TH1F("h318", "Y Dev 3  234 ", 51, -1.02, 1.02);

TH1F *h380 = new TH1F("h380", " Raw Hit/Event ", 201, -0.5, 200.5);

TH1F *h381 = new TH1F("h381", " Hits/Event after Tcut ", 101, -0.5, 100.5);

TH1F *h390 = new TH1F("h390", " Times after Tcut ", 51, -25.5, 25.5);

TH1F *h391 = new TH1F("h391", " Times before Tcut ", 401, -0.5, 400.5);

TH1F *h392 = new TH1F("h392", " Offset Times before Tcut ", 201, -50.5, 150.5);



int  ncalltk, MaxEvent;
float window;
int itz[16], itwin;
float zz[4], xx[4], yy[4];
static const int   MaxHit = 200;
int   EvW[MaxHit], EvT[MaxHit], nhit;

//                    note indexing is [x,y]
int   nxyhit[2][4];
float posxy[2][4][10];
int   itsvxy[2][4][10];

int       startcput, endcput;

//======================  Track Finding  =====================

int SLFTC(int npts, float& a, float& b, float& chs, float x[], float z[])
{
	float  t1, t2, t3, t4, denom, xn;
	float  sig, xt;
	//                   COMPUTE STRAIGHT LINE   X = A + B * Z
	//                   SIMPLE FIT, NO ERRORS INCLUDED OR GENERATED
	//==================================================================
	sig = 0.1 / 3.46;
	xn = npts;
	a = 0.0;  b = 0.0;
	t1 = 0.0; t2 = 0.0; t3 = 0.0; t4 = 0.0;

	for (int i = 0; i < npts; i++)
	{
		t1 = t1 + z[i];
		t2 = t2 + x[i] * z[i];
		t3 = t3 + z[i] * z[i];
		t4 = t4 + x[i];
	}

	denom = t1 * t1 - xn * t3;
	if (abs(denom) < 0.0000001) return 1;
	a = (t1 * t2 - t3 * t4) / denom;
	b = (t1 * t4 - xn * t2) / denom;

	//                                         calculate chisq for the fit
	chs = 0.0;
	for (int i = 0; i < npts; i++)
	{
		xt = (a + b * z[i] - x[i]);
		chs = chs + (x[i] - xt)*(x[i] - xt);
	}
	if (chs > 19.0) chs = 19.0;
	return 0;
}

//========================================================================   

void tkf4h(float www, int ixy, int nhit[4], float xx[10][4], int itim[10][4],
	int& ntk, float xs[5], float xi[5], float chsq[5], int knd[5], float tkhit[5][4], int itmtk[5][4])
{
	//      track finder for 4 planes
	//     use ihoff to distinguish x and y calls.
	//     zz is in global, so use that.  Note the reuse here of xx !
	//     histograming not yet implemented  0 for x, 20 for y not yet

	int  maxtk = 5;
	float tmp = 0.0;

	//    float at, bt, z1, z2, x1, x2, dx, dz;
	//    int  jp1, jp2, k, npr, im, npr, k;
	float  xt[4], zt[4], xtry, zrat2, zrat3;
	float  window, dev2s, dev3s, devs;
	int  n1, n2, n3, n4;
	int  iuse[10][4], i1s, i2s, i3s, i4s;
	int   ksv2, ksv3;
	float   win, dev2, dev3, dev, den;
	int itmp, kind[5], jgot;
	//                                   more for 3/4 search
	//                           ip( plane, cycle )
	//      int ip[3][4] = {1,2,3, 1,2,4,  1,3,4,  2,3,4 }
	//      int m1, m2, m3, m, ksv, ksv2, ksv3;

	int kknd[4] = { 4, 3, 2, 1 };
	int ip[3][4] = { { 0, 0, 0, 1}, { 1, 1, 2, 2 }, {  2, 3, 3, 3 } };
	int m1, m2, m3, ksv;

	//   so as to not reuse tracks,  iuse(hit,plane) is set to the track number
	//    there will be 4 possible miss locations, as only allow one miss
	//    on a track if 4 chambers...

	//  note - loop takes first hit in window, not best.  Should upgrade....
	
	//...........................................................
	ncalltk++;      ntk = 0;

	if (ncalltk < -3)
	{
		std::cout << " nhit  " << nhit[0] << "   " << nhit[1] << "   " <<
			nhit[2] << "   " << nhit[3] << std::endl;
		std::cout << " zz  " << zz[0] << "  " << zz[1] << "   "
			<< zz[2] << "  " << zz[3] << std::endl;
		std::cout << " pos  " << xx[0][0] << "  " << xx[0][1] << "   "
			<< xx[0][2] << "  " << xx[0][3] << std::endl;
	}


	//      window = www;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			iuse[j][i] = 0;
		}
	}

	tmp = -77.0; itmp = -30;
	for (int i = 0; i < maxtk; i++)
	{
		xi[i] = 0.0;   xs[i] = 0.0, kind[i] = 0;
		for (int j = 0; j < 4; j++)
		{
			tkhit[i][j] = tmp;
			itmtk[i][j] = itmp;
		}
	}
	//                        search for plane 1 to 4 tracks

	n1 = nhit[0];
	if (n1 > 9) n1 = 9;
	n2 = nhit[1];
	if (n2 > 9) n2 = 9;
	n3 = nhit[2];
	if (n3 > 9) n3 = 9;
	n4 = nhit[3];
	if (n4 > 9) n4 = 9;
	den = zz[3] - zz[0];
	if (abs(den) < 10.0) den = 100.0;
	zrat2 = (zz[1] - zz[0]) / (zz[3] - zz[0]);
	zrat3 = (zz[2] - zz[0]) / (zz[3] - zz[0]);

	if (ncalltk < -3) {
		std::cout << "  zrat2, zrat3 = " << zrat2 << "    " << zrat3 << std::endl;
		std::cout << std::endl << " zrat 2 3 = " << zrat2 << "   " << zrat3 << std::endl << std::endl;
		std::cout << std::endl << " x array " << std::endl;
		std::cout << "  " << xx[0][0] << "  " << xx[0][1] << "  " << xx[0][2] <<
			"  " << xx[0][3] << std::endl;
	}

	for (int i1 = 0; i1 < n1; i1++)
	{
		if (iuse[i1][0] > 0) goto L_ch1;
		xt[0] = xx[i1][0];     i1s = i1;

		for (int i4 = 0; i4 < n4; i4++)
		{
			if (iuse[i4][3] > 0) goto L_ch4;
			i4s = i4;   xt[3] = xx[i4][3];
			xtry = xt[0] + (xt[3] - xt[0])*zrat2;
			win = www;
			ksv2 = -5;

			if (ncalltk < -3) {
				std::cout << "   ref points xt0, xt3 " << xt[0] << "   " << xt[3] << std::endl;
				std::cout << "    ch2 search  xtry = " << xtry << std::endl;
				//                                           find conf hit in ch2
				std::cout << std::endl << "  checking ch 2 " << std::endl;
			}

			dev2s = -5.0;
			for (int i2 = 0; i2 < n2; i2++)
			{
				if (iuse[i2][1] > 0) break;
				xt[1] = xx[i2][1];
				dev2 = xt[1] - xtry;

				if (ncalltk < -3) { std::cout << "  dev2 = " << dev2 << std::endl; }

				if (abs(dev2) <= win)
				{
					ksv2 = i2; win = abs(dev2); dev2s = dev2;
				}
			}
			if (ksv2 < 0) goto L_ch4;
			if (ixy == 1) h301->Fill(dev2s);
			if (ixy == 2) h311->Fill(dev2s);

			xt[2] = xx[ksv2][2];
			xtry = xt[0] + (xt[3] - xt[0])*zrat3;
			win = www;
			ksv3 = -5;
			dev3s = -5.0;

			//   std::cout <<std::endl <<std::endl << "    ch3 search  xtry = " << xtry <<std::endl;

			for (int i3 = 0; i3 < n3; i3++)
				//                                              find conf. hit in ch 3
			{
				if (iuse[i3][2] > 0) break;
				xt[3] = xx[i3][2];
				dev3 = xt[2] - xtry;

				if (ncalltk < -3) { std::cout << "  dev3 = " << dev3 << std::endl; }

				if (abs(dev3) < win)
				{
					win = abs(dev3);  ksv3 = i3; dev3s = dev3;
				}
			}
			if (ksv3 < 0) goto L_ch4;
			if (ixy == 1) h302->Fill(dev3s);
			if (ixy == 2) h312->Fill(dev3s);
			//                                                save the 4 pt track                  
			xt[2] = xx[ksv3][2];
			if (ntk > 5) ntk = 5;
			SLFTC(4, xi[ntk], xs[ntk], chsq[ntk], xt, zz);
			kind[ntk] = 5;
			iuse[i1s][0] = ntk + 1;
			iuse[ksv2][1] = ntk + 1;
			iuse[ksv3][2] = ntk + 1;
			iuse[i4s][3] = ntk + 1;
			tkhit[ntk][0] = xt[0];
			tkhit[ntk][1] = xt[1];
			tkhit[ntk][2] = xt[2];
			tkhit[ntk][3] = xt[3];
			knd[ntk] = 5;
			itmtk[ntk][0] = itim[i1s][0];
			itmtk[ntk][1] = itim[ksv2][1];
			itmtk[ntk][2] = itim[ksv3][2];
			itmtk[ntk][3] = itim[i4s][3];
			ntk++;   if (ntk > 4) ntk = 4;

		L_ch4: itmp = 4;
		}                  // chamber 4  loop end
	L_ch1: itmp = 1;
	}                   //  chamber 1 loop end


 //                             find 3 point tracks
	for (int m = 0; m < 4; m++)
	{
		m1 = ip[0][m];
		zt[0] = zz[m1];
		n1 = nhit[m1];
		if (n1 > 10) n1 = 9;
		for (int i1 = 0; i1 < n1; i1++)
		{
			if (iuse[i1][m1] > 0) goto L_510i1;
			xt[0] = xx[i1][m1];
			i1s = i1;
			m3 = ip[2][m];
			n3 = nhit[m3];
			zt[2] = zz[m3];
			if (n3 > 9) n3 = 9;

			for (int i3 = 0; i3 < n3; i3++)
			{
				if (iuse[i3][m3] > 0) goto L_530i3;
				xt[2] = xx[i3][m3];
				i3s = i3;
				m2 = ip[1][m];
				zt[1] = zz[m2];
				zrat2 = (zt[1] - zt[0]) / (zt[2] - zt[0]);
				xtry = xx[i1][m1] + (xx[i3][m3] - xx[i1][m1])*zrat2;
				n2 = nhit[m2];
				if (n2 > 9) n2 = 9;
				win = www;
				ksv = -5; jgot = 0;
				//                                            confirming middle hit?
				for (int i2 = 0; i2 < n2; i2++)
				{
					if (iuse[i2][m2] > 0) break;
					i2s = i2;
					xt[1] = xx[i2][m2];
					dev = xt[1] - xtry;

					if (abs(dev) <= win)
					{
						win = abs(dev);
						devs = dev;
						ksv = i2; jgot = 5;
					}
				}
				if (jgot <= 0) goto L_530i3;
				xt[1] = xx[ksv][m2];
				SLFTC(3, xi[ntk], xs[ntk], chsq[ntk], xt, zt);
				kind[ntk] = 4 - m;
				iuse[i1][m1] = ntk + 1;
				iuse[ksv][m2] = ntk + 1;
				iuse[i3][m3] = ntk + 1;
				knd[ntk] = kknd[m];
				tkhit[ntk][m1] = xt[0];
				tkhit[ntk][m2] = xt[1];
				tkhit[ntk][m3] = xt[2];
				itmtk[ntk][m1] = itim[i1s][m1];
				itmtk[ntk][m2] = itim[i2s][m2];
				itmtk[ntk][m3] = itim[i3s][m3];
				ntk++;   if (ntk > 4) ntk = 4;
				SLFTC(4, xi[ntk], xs[ntk], chsq[ntk], xt, zt);
				if (ixy == 1 && m == 0) h305->Fill(devs);
				if (ixy == 1 && m == 1) h306->Fill(devs);
				if (ixy == 1 && m == 2) h307->Fill(devs);
				if (ixy == 1 && m == 3) h308->Fill(devs);

				if (ixy == 2 && m == 0) h315->Fill(devs);
				if (ixy == 2 && m == 1) h316->Fill(devs);
				if (ixy == 2 && m == 2) h317->Fill(devs);
				if (ixy == 2 && m == 3) h318->Fill(devs);

				goto L_510i1;


			L_530i3: itmp = 3;
			}                  // bottom of i3 loop
		L_510i1:  itmp = 1;
		}                        // bottom of i1 loop
	}                          // bottom of m loop

//    if( ntk > 0 )
//    {
//      for( i = 0; i < ntk; i++ )
//      { tmp = kind[i];
//        h351 -> Fill(tmp);
//      }
//    }

//    std::cout <<std::endl << " Track Find ntk = " << ntk << " kind 0,1 = " << kind[0]
//      << "  " << kind[1] <<std::endl;

	return;
}



//====================  chamber to xy space ===========
void wtoxy(void)
//  from wire hit arrays make x,y arrays
//  assumes time cuts done, just save them
//  note   0 <= ipl <= 7
{
	//  int  MaxPt = 10;
	//  ary[3][4] has 3 rows of 4 colums
	// positoin of wire 0 [x,y][ch no] four ch spr, summer 2015
	//    198.8     475.0    758.8     1534.5                      zz
	//     5.97      6.75     6.80       6.80                      xi
	//      6.5      6.60      6.4       6.5                       yi
	//  float xyi[2][4];
	//  float posxy[2][4][MaxPt=10];
	//  int   itsvxy[2][4][MaxPt];
	//----------------------------------------------------------

	float posi[2][4];

	for (int i = 0; i < 4; i++)
	{
		posi[0][i] = xx[i]; posi[1][i] = yy[i];
		nxyhit[0][i] = 0;   nxyhit[1][i] = 0;

		for (int j = 0; j < 10; j++)
		{
			posxy[0][i][j] = -77.0;  posxy[1][i][j] = -77.0;
			itsvxy[0][i][j] = -70;  itsvxy[1][i][j] = -70;
		}
	}

	if (nhit < 1) return;

	for (int ihit = 0; ihit < nhit; ihit++)
	{
		int ipl = EvW[ihit] / 128;
		int iwpl = EvW[ihit] - ipl * 128;
		int kplv = ipl % 2;     // = remainder, 0>0, 1>1, 2>0, 3>1 ...
		int jplc = ipl / 2;     // = x, y  0>0, 1>0, 2>1, 3>1, 4>2 ... 7>4


		if (nxyhit[kplv][jplc] > 9) { nxyhit[kplv][jplc] = 9; }
		int nn = nxyhit[kplv][jplc];

		posxy[kplv][jplc][nn] = iwpl * 0.1 - posi[kplv][jplc];
		itsvxy[kplv][jplc][nn] = EvT[ihit];
		nxyhit[kplv][jplc]++;
	}
	//  return;
}

//===========================   Setup  =======================

void Setup(void)
//            read setup information
//        Loop over the lines of input ( index il )
//        read stuff, and print it out, line by line
//        geometry parameters are global
{
	std::string dbuf;
	std::string datfile;
	int i, j, il;
	std::ifstream setupin;
	int mode;

	setupin.open("anatk.inp");
	std::cout << std::endl;
	std::cout << " reading Setup Info from  anatk.inp" << std::endl << std::endl;

	//                            loop over input lines = 10 ( 0... 9 )
	for (il = 0; il < 10; il++)
	{
		getline(setupin, dbuf);
		std::istringstream ebuf(dbuf.c_str());

		if (il == 0) {
			ebuf >> datfile;
			std::cout << std::endl << "  Input file is >" << datfile;
		}

		//              mode = 1, use the above input file
		//                   > 1  use self ( select file from list )
		if (il == 1)
		{
			ebuf >> mode;
			std::cout << std::endl;
			std::cout << std::endl << " File Select mode = " << mode << std::endl;
		}

		if (il == 2)
		{
			ebuf >> MaxEvent;
			std::cout << std::endl;
			std::cout << std::endl << " Maximum Number of Events = " << MaxEvent << std::endl;
		}

		if (il == 3)
		{
			for (i = 0; i < 4; i++)
			{
				ebuf >> zz[i];
			}
			std::cout << std::endl << "  zz  = ";
			for (j = 0; j < 4; j++)
			{
				std::cout << "   " << zz[j];
			}
			std::cout << std::endl;
		}

		if (il == 4)
		{
			for (i = 0; i < 4; i++)
			{
				ebuf >> xx[i];
			}
			std::cout << std::endl << "  xx  = ";
			for (j = 0; j < 4; j++)
			{
				std::cout << "   " << xx[j];
			}
			std::cout << std::endl;
		}

		if (il == 5)
		{
			for (i = 0; i < 4; i++)
			{
				ebuf >> yy[i];
			}
			std::cout << std::endl << "  yy  = ";
			for (j = 0; j < 4; j++)
			{
				std::cout << "   " << yy[j];
			}
			std::cout << std::endl;
		}

		if (il == 6)
		{
			ebuf >> window;
			std::cout << std::endl;
			std::cout << std::endl << " search window (+/-) = " << window << " cm " << std::endl;
		}
		int it;
		if (il == 7)
		{
			for (it = 0; it < 8; it++)
			{
				ebuf >> itz[it];
			}
			std::cout << std::endl << "  t cent  = ";
			for (j = 0; j < 8; j++)
			{
				std::cout << "   " << itz[j];
			}
			std::cout << std::endl;
		}
		if (il == 8)
		{
			for (it = 8; it < 16; it++)
			{
				ebuf >> itz[it];
			}
			std::cout << std::endl << "  t cent  = ";
			for (j = 8; j < 16; j++)
			{
				std::cout << "   " << itz[j];
			}
			std::cout << std::endl;
		}
		if (il == 9)
		{
			ebuf >> itwin;
			std::cout << std::endl;
			std::cout << std::endl << " time search window (+/-) "
				<< itwin << " time bins " << std::endl;
		}

	}                    // end of lines of input loop
	setupin.close();
	return;
}



//===========================================================================
//                  time cuts.   numbers in global


void tcut(void)
//     make the time cut  - Just time cuts
//    Copy the result back to original array
//     see doug_32\WC\NewTk+rd44\  and \rd
{
	int  itdc, i, ihit, jw, jt;
	int  mhit, Iwc[MaxHit], Itc[MaxHit];

	mhit = 0;
	for (i = 0; i < MaxHit; i++)
	{
		Iwc[i] = 0; Itc[i] = 0;
	}

	if (nhit < 1) return;
	//                                      timing cut >> Iwc, Itc
	for (ihit = 0; ihit < nhit; ihit++)
	{

		jw = EvW[ihit];  itdc = jw / 64;
		jt = EvT[ihit];
		h391->Fill(jt);
		jt = jt - itz[itdc];
		h392->Fill(jt);
		if (jt > -itwin && jt < itwin)
		{
			Iwc[mhit] = jw;  Itc[mhit] = jt;
			h390->Fill(jt);
			mhit++;
			if (mhit >= MaxHit - 1) mhit = MaxHit - 1;
		}
	}
	//  std::cout << "  " <<std::endl;
	//  std::cout << " tcut says nhit, mhit = " << nhit << "  " << mhit <<std::endl;

	if (mhit > 0)
	{
		nhit = mhit;
		for (i = 0; i <= mhit; i++)
		{
			EvW[i] = Iwc[i];
			EvT[i] = Itc[i];
		}
	}

	//    return;
}


//==================================================================================
//                     print hit arrays
void pospr(void)
{
	//	 int iv, ic, ih;
	int  nn, k;
	nn = 1;
	if (nxyhit[0][0] > nn) nn = nxyhit[0][0];
	if (nxyhit[0][1] > nn) nn = nxyhit[0][1];
	if (nxyhit[0][2] > nn) nn = nxyhit[0][2];
	if (nxyhit[0][3] > nn) nn = nxyhit[0][3];

	std::cout << std::endl << " Chamber Hits (from pospr) " << std::endl;
	std::cout << " ch no       1      2      3      4 " << std::endl;
	//   for( iv = 0; iv<2; iv++ )
	std::cout << " N X hits    " << nxyhit[0][0] << "      " << nxyhit[0][1]
		<< "      " << nxyhit[0][2] << "      " << nxyhit[0][3] << std::endl;

	for (k = 0; k < nn; k++)
	{
		std::cout << " X hits   " << posxy[0][0][k] << "    " << posxy[0][1][k]
			<< "    " << posxy[0][2][k] << "    " << posxy[0][3][k] << std::endl;
	}

	std::cout << " N Y hits    " << nxyhit[1][0] << "      " << nxyhit[1][1]
		<< "      " << nxyhit[1][2] << "      " << nxyhit[1][3] << std::endl;
	std::cout << " Y hits   " << posxy[1][0][0] << "    " << posxy[1][1][0]
		<< "    " << posxy[1][2][0] << "    " << posxy[1][3][0] << std::endl;
}
//======================================================================
void anatka(void)
{

	TFile  *anatf = new TFile("anat.root", "RECREATE");

	//    datfile  is the data file to read from
	//    infile has the name of the datfile
	//   ---------- make histogram definitions look like below to compile
	//  TH1F *h1 = new TH1F("h1","x distribution",100,-4,4);

	std::ifstream  datfile;
	std::ifstream  infile;

	// filename is the name of the file, read in from infile
	// FullLine is the string into which the real data is read.
	std::string    FullLine;
	std::string    filename;
	std::string    label;
	//int  ic, ires, c, x, y, Ipl, Iw;

	int       Ldone, Nev, NevSp;
	int       In1, In2, In3;
	int       InNev, InNevsp;
	int       Nspill, InNspill;
	std::string    spDate, spTime;
	float     etime, detime;
	int       IplHit[8], Jwhit[8], JplHit[8];

	TH1     *h150[8];
	TH1     *h160[8];
	TH1     *h170[8];
	TH1     *h200[16];
	TString  hname, htit, hhnam, htname;

	int       EvGoing, IsValid;
	int       MTdc, InChan, InTime;
	int       ipl, iwpl, itdc;
	int       ConSpMo, ConSpDay, ConSpYr, ConSpHr, ConSpMin, ConSpSec;
	float     Etime, EtimeLast, dEtime;
	int       JEt1, JEt2, FoundEtime;
	float     www, pos[10][4], xs[5], xi[5], ys[5], yi[5], chsxt[5], chsyt[5];
	int       mhit[4], itim[10][4], timhitx[5][4], timhity[5][4];
	int       ntkx, ntky, ixy;
	float      tkhitx[5][4], tkhity[5][4];
	int   kindx[5], kindy[5];
	float  chsx, chsy, xxs, xxi, yys, yyi, kindxx, kindyy;

	TStopwatch timer;
	timer.Start();

	TH1F *h100 = new TH1F("h100", "All Wire Hits", 128, -0.5, 1023.5);
	h100->SetXTitle(" Wire Number");
	h100->SetFillColor(3);

	TH1F *h120 = new TH1F("h120", "Event Time", 600, 0.0, 6.0);
	h120->SetFillColor(3);
	TH1F *h121 = new TH1F("h121", "Time bet. Ev", 250, 0.0, 0.025);
	h121->SetFillColor(3);
	TH1F *h130 = new TH1F("h130", "Events per Spill", 251, -0.5, 251.5);
	h130->SetFillColor(3);

	h100->SetFillColor(3);

	h301->SetFillColor(3);
	h302->SetFillColor(3);
	h305->SetFillColor(3);
	h306->SetFillColor(3);
	h307->SetFillColor(3);
	h308->SetFillColor(3);



	h311->SetFillColor(4);
	h312->SetFillColor(4);
	h315->SetFillColor(4);
	h316->SetFillColor(4);
	h317->SetFillColor(4);
	h318->SetFillColor(4);


	//                             for each wire plane = X1, Y1, X2 ...
	std::string lblXY[2];    lblXY[0] = "X";  lblXY[1] = "Y";
	int ich;

	for (int i = 0; i < 8; i++)
	{
		std::stringstream sstm;
		sstm << "h15" << i;
		hname = sstm.str();
		ich = i / 2 + 1;
		int j = i % 2;
		sstm << " Wire Chamber " << ich << " " << lblXY[j];
		htit = sstm.str();
		h150[i] = new TH1F(hname, htit, 128, -0.5, 127.5);
		h150[i]->SetXTitle(" Wire No");
		h150[i]->SetFillColor(3);
	}

	for (int i = 0; i < 8; i++)
	{
		std::stringstream  sstn, sttx;
		sstn << "h16" << i;
		hhnam = sstn.str();
		ich = i / 2 + 1;
		int j = i % 2;
		sttx << " Hits " << ich << lblXY[j];
		htit = sttx.str();

		h160[i] = new TH1F(hhnam, htit, 11, -0.5, 10.5);
		h160[i]->SetFillColor(3);

		h170[i] = new TH1F(hhnam, htit, 11, -0.5, 10.5);
		h170[i]->SetFillColor(3);
	}

	TH1F *h110 = new TH1F("h110", "All Time Hits", 301, -0.5, 300.5);
	h110->SetXTitle("Time Bins");
	h110->SetFillColor(3);


	TH2F *h181 = new TH2F("h181", " Chamber 1", 128, -0.5, 127.5, 128, -0.5, 127.5);
	h181->SetMarkerStyle(20);
	h181->SetMarkerSize(0.25);
	h181->SetMarkerColor(3);
	TH2F *h182 = new TH2F("h182", " Chamber 2", 128, -0.5, 127.5, 128, -0.5, 127.5);
	h182->SetMarkerStyle(20);
	h182->SetMarkerSize(0.25);
	h182->SetMarkerColor(3);
	TH2F *h183 = new TH2F("h183", " Chamber 3", 128, -0.5, 127.5, 128, -0.5, 127.5);
	h183->SetMarkerStyle(20);
	h183->SetMarkerSize(0.25);
	h183->SetMarkerColor(3);
	TH2F *h184 = new TH2F("h184", " Chamber 4", 128, -0.5, 127.5, 128, -0.5, 127.5);
	h184->SetMarkerStyle(20);
	h184->SetMarkerSize(0.25);
	h184->SetMarkerColor(3);

	for (int i = 0; i < 16; i++)
	{
		std::stringstream strt, sttt;
		if (i < 10) strt << "h20" << i;
		if (i > 9) strt << "h2" << i;
		htname = strt.str();

		sttt << " M TDC " << i;
		htit = sttt.str();
		h200[i] = new TH1F(htname, htit, 301, -0.5, 300.5);
		h200[i]->SetXTitle("Time Bins");
		h200[i]->SetFillColor(3);
	}

	TH2F *h320 = new TH2F("h320", " X(6) Y(5) Tracks", 7, -0.5, 6.5, 6, -0.5, 5.5);
	h320->SetFillColor(3);

	TH1F *h321 = new TH1F("h321", " X Tracks ", 6, -0.5, 5.5);
	h321->SetFillColor(3);
	TH1F *h322 = new TH1F("h322", " Y Tracks ", 6, -0.5, 5.5);
	h322->SetFillColor(4);

	TH1F *h341 = new TH1F("h341", " X Track Slope ", 51, -0.005, 0.005);
	h341->SetFillColor(3);
	TH1F *h342 = new TH1F("h342", " X Track Intercept ", 65, -6.5, 6.5);
	h342->SetFillColor(3);
	TH1F *h343 = new TH1F("h343", " Y Track Slope ", 51, -0.005, 0.005);
	h343->SetFillColor(4);
	TH1F *h344 = new TH1F("h344", " Y Track Intercept ", 65, -6.5, 6.5);
	h344->SetFillColor(4);

	//====================================================================//

	std::cout << std::endl;
	std::cout << "  Calling Setup " << std::endl;
	std::cout << std::endl;

	Setup();

	//=================    off to the analysis  ==========================//


	//   first get the name of the file 
	infile.open("ifile.dat");
	getline(infile, filename);
	infile.close();
	std::cout << std::endl << "  ifile as read >" << filename << std::endl << std::endl;

	datfile.open(filename.c_str());

	// datfile.open( "pi32.dat" );

	//=========================================================
	// for tracking ntuple  
	//  Nev, NevSp, IsValid, ntkx, ntky, kindx, kindy
	//  detime, detime      <<< need implementing yet///
	//  xs, xi, chsx, ys, yi, chsy
	//   TNtuple *ntuple =
	//      new TNtuple("ntuple","data from ascii file","x:y:z");
	// new TNtuple("trk","StLine","Nev:NevSp:IsValid:ntkx:ntky:kindx:kindy:xs:xi:chsx:ys:yi:chsy");
	//    all floats in this simple ntuple.
	//    ntuple->Fill(x,y,z);    // and that should be that....

	TNtuple *trk = new TNtuple("trk", "StLine", "Nev:NevSp:IsValid:ntkx:ntky:kindxx:kindyy:xxs:xxi:chsx:yys:yyi:chsy");

	//

	Ldone = 0;
	Nev = 0;
	ncalltk = 0;
	NevSp = 0;
	Nspill = 0;
	EvGoing = 0;
	IsValid = 0;
	nhit = 0;
	FoundEtime = 0;
	EtimeLast = -1.0;
	for (int i = 0; i < 8; i++) { IplHit[i] = 0; JplHit[i] = 0; }

	while (!datfile.eof())
	{

		for (int ii = 0; ii < 11; ii++)
		{
			getline(datfile, FullLine);
			//     if( Ldone < 100 ) std::cout << " line read: " << FullLine <<std::endl;
			Ldone++;
			std::istringstream line(FullLine.c_str());

			line >> label;

			if (label == "SPILL")
			{
				line >> InNspill;  Nspill++;
				int a = Nspill; 
				int b = NevSp;   
				h130->Fill(a, b);

				std::cout << " New Spill number " << Nspill << "  NevSp = " << NevSp << std::endl;
				NevSp = 0;
			}

			if (label == "SDATE")
			{
				line >> In1; line >> In2;  line >> In3;
				ConSpDay = In1; ConSpMo = In2; ConSpYr = In3;
			}

			if (label == "STIME")
			{
				line >> In1;  line >> In2;  line >> In3;
				ConSpHr = In1;  ConSpMin = In2;  ConSpSec = In3;
			}

			if (label == "TTIME")
			{
				line >> spDate;  line >> spTime;
				//      std::cout << " Spill Date " << spDate << "     Spill Time " << spTime;
				//      std::cout <<std::endl;  }
			}

			if (label == "ETIME")
			{
				line >> JEt1; line >> JEt2;  Etime = JEt2 * 9.14E-9;
				h120->Fill(Etime);
				if (FoundEtime > 0)
				{
					dEtime = Etime - EtimeLast;
					h121->Fill(dEtime);
				}
				FoundEtime = 1;  EtimeLast = Etime;
			}


			if (label == "Module")     //  chamber TDC number 
			{
				line >> In1;   MTdc = In1;
			}

			if (label == "Channel")    // which wire hit, at what time
			{
				line >> InChan;  line >> InTime;

				EvW[nhit] = InChan + 64 * (MTdc - 1);
				EvT[nhit] = InTime;
				nhit++;    if (nhit > 199) nhit = 199;
			}

			//  -------------------------------   EVENT processing --------------     	        
			if (label == "EVENT")
			{
				line >> In1;  line >> In2;
				InNev = In1; InNevsp = In2;
				//      std::cout <<std::endl << "  line " << Ldone << "   Got an EVENT " <<std::endl; 
				if (EvGoing)
				{
					Nev++;  NevSp++;  IsValid = 0;
					if (Nev % 1000 == 1) std::cout << "  Event Number " << Nev << std::endl;
					//       std::cout <<std::endl;  std::cout << " ready with event, nhit = " << nhit  <<std::endl;

					if (Nev < -3)       // Take a look at an event  
					{
						int nn; nn = nhit;
						//     int j;
						if (nn < 1) nn = 1;    
						if (nn > 30) nn = 30;
						std::cout << " Event Number " << Nev << "   nhits =" << nhit << std::endl;
						for (int j = 0; j < nn; j++)
						{
							std::cout << " j " << j << " wire " << EvW[j] << std::endl;
						}
					}
					h380->Fill(nhit);
					for (int k = 1; k < nhit; k++)
					{
						int w = EvW[k];   
						h100->Fill(w);

						int t = EvT[k];   
						h110->Fill(t);

						ipl = EvW[k] / 128; 
						IplHit[ipl]++;
						iwpl = EvW[k] - 128 * (ipl); 
						Jwhit[ipl] = iwpl;
						int a = iwpl;
						itdc = EvW[k] / 64;

						h150[ipl]->Fill(a);
						if (itdc >= 0 && itdc < 16)  h200[itdc]->Fill(t);
					}

					//                                for all hits.  Not many one and only 1 hit events/plane....  
					//                                similar code below for hits after time cuts       
					//       for( int j=0; j<4; j++ )
					//        { ix = 2*j;    iy = ix+1;
					//           if( IplHit[ix] == 1 && IplHit[iy] == 1 )
					//           { a = Jwhit[ix]; b = Jwhit[iy]; 
					//               if( j == 0 ) h181 -> Fill(a,b);
					//               if( j == 1 ) h182 -> Fill(a,b);
					//               if( j == 2 ) h183 -> Fill(a,b);
					//               if( j == 3 ) h184 -> Fill(a,b);   
					//           }
					//         }

					tcut();  
					h381->Fill(nhit);

					wtoxy();

					for (int k = 1; k < nhit; k++)
					{
						int w = EvW[k];  
						int t = EvT[k];
						ipl = EvW[k] / 128;  
						JplHit[ipl]++;
						iwpl = EvW[k] - 128 * (ipl); 
						Jwhit[ipl] = iwpl;
					}


					for (int j = 0; j < 4; j++)
					{
						int ix = 2 * j;   
						int iy = ix + 1;
						if (JplHit[ix] == 1 && JplHit[iy] == 1)
						{
							int a = Jwhit[ix]; 
							int b = Jwhit[iy];
							if (j == 0) h181->Fill(a, b);
							if (j == 1) h182->Fill(a, b);
							if (j == 2) h183->Fill(a, b);
							if (j == 3) h184->Fill(a, b);
						}
					}




					if (Nev < -3) pospr();

					www = 0.5;
					ntkx = 0;
					ixy = 1;
					for (int i = 0; i < 5; i++)
					{
						xs[i] = 0.0; xi[i] = 0.0, kindx[i] = 0, chsxt[i] = -1.0;
						for (int j = 1; j < 4; j++)
						{
							timhitx[i][j] = 0;
						}
					}

					for (int i = 0; i < 4; i++)
					{
						mhit[i] = nxyhit[0][i]; mhit[i] = 1;
						//            for( j=0; j<nmhit[i]; j++ )
						{ pos[0][i] = posxy[0][i][0];
						itim[0][i] = itsvxy[0][i][0]; }
					}

					for (int k = 0; k < 4; k++)
					{
						mhit[k] = nxyhit[0][k];
						pos[0][k] = posxy[0][k][0];
					}


					tkf4h(www, ixy, mhit, pos, itim, ntkx, xs, xi, chsxt, kindx, tkhitx, timhitx);

					h321->Fill(ntkx);
					xxs = xs[0];
					xxi = xi[0];
					if (ntkx >= 1)
					{
						h341->Fill(xxs);
						h342->Fill(xxi);
					}

					ntky = 0;
					ixy = 2;
					for (int i = 0; i < 5; i++)
					{
						ys[i] = 0.0; yi[i] = 0.0; kindy[i] = 0; chsyt[i] = -1.0;
						for (int j = 1; j < 4; j++)
						{
							timhity[i][j] = 0;
							tkhity[i][j] = 0.0;
						}
					}

					//         for( i=0; i++; i<4 )
					//          {  mhit[i] = nxyhit[1][i]; mhit[i] = 1;
					//            for( j=0; j++; j<nmhit[i] )
					//            { pos[0][i] = posxy[1][i][0];
					//             itim[0][i] = itsvxy[1][i][0]; }
					//          }   

					for (int k = 0; k < 4; k++)
					{
						mhit[k] = nxyhit[1][k];
						pos[0][k] = posxy[1][k][0];
						itim[0][k] = itsvxy[1][k][0];
					}


					tkf4h(www, ixy, mhit, pos, itim, ntky, ys, yi, chsyt, kindy, tkhity, timhity);

					h322->Fill(ntky);
					yyi = yi[0];
					yys = ys[0];
					if (ntky >= 1)
					{
						h343->Fill(yys);
						h344->Fill(yyi);
					}

					h320->Fill(ntkx, ntky);

					if ((ntkx > 0) & (ntky > 0)) IsValid = 1;

					chsx = chsxt[0]; chsy = chsyt[0];
					kindyy = kindy[0]; if (kindyy > 10) kindyy = 10; kindxx = kindx[0]; if (kindxx > 10) kindxx = 10;

					trk->Fill(Nev, NevSp, IsValid, ntkx, ntky, kindxx, kindyy, xxs, xxi, chsx, yys, yyi, chsy);


					for (int j = 0; j < 8; j++)
					{
						int a = IplHit[j]; 
						h160[j]->Fill(a); 
						IplHit[j] = 0;
						a = JplHit[j]; 
						h170[j]->Fill(a); 
						JplHit[j] = 0;
					}
					nhit = 0;
				}
				EvGoing = 1;
				nhit = 0;
				if (Nev >= MaxEvent) goto done;
			}
		}
	}



done:  std::cout << "  Done, Nev = " << Nev << std::endl << std::endl;
	datfile.close();

	timer.Stop();
	std::cout << std::endl;
	std::cout << "   CPU time used " << timer.CpuTime() << " seconds " << std::endl << std::endl;

	std::cout << std::endl;
	std::cout << "  Finished Reading   " << Ldone << "  lines" << std::endl;
	std::cout << "  Number of Spills   " << Nspill << std::endl;
	std::cout << "  Number of Events   " << Nev;
	std::cout << std::endl << std::endl;

	//   Timing the file pi32.dat 
	//   without compiling     100 events use 48 sec
	//           compiling    1000 events use .06 sec   !


	//                           save the histograms
	//  trk -> Write();
	h301->Write(); h302->Write(); h311->Write(); h312->Write();
	h305->Write(); h306->Write(); h307->Write(); h308->Write();
	h315->Write(); h316->Write(); h317->Write(); h318->Write();
	h380->Write(); h381->Write(); h390->Write(); h391->Write(); h392->Write();

	anatf->Write();


}



