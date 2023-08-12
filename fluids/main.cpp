/*
  FLUIDS v.3 - SPH Fluid Simulator for CPU and GPU
  Copyright (C) 2012. Rama Hoetzlein, http://www.rchoetzlein.com

  ZLib license
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/********************************************************************************************/
/*   PCISPH is integrated by Xiao Nie for NVIDIA��s 2013 CUDA Campus Programming Contest    */
/*                     https://github.com/Gfans/ISPH_NVIDIA_CUDA_CONTEST                    */
/*   For the PCISPH, please refer to the paper "Predictive-Corrective Incompressible SPH"   */
/********************************************************************************************/

#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "common_defs.h"
#include "camera3d.h"

#ifdef USE_CUDA
#include "fluid_system_host.cuh"	
#endif
#include "fluid_system.h"
#include "gl_helper.h"

#include <gl/glut.h>

// ȫ�ֱ���
Camera3D	g_camera;
ParticleSystem	g_fluid_system;

float		g_window_width  = 1024;
float		g_window_height = 768;			// height:width = 4:3

Vector4DF	g_light[2];						// �ƹ�
Vector4DF	g_light_to[2];					
float		g_light_fov = 45.0f;	

Vector3DF	g_obj_from;
Vector3DF   g_obj_angles;
Vector3DF	g_obj_dang;

int			g_fluid_system_rate = 0;		// ����
int			g_fluid_system_freq = 1;

bool		g_help = false;					
int			g_shade = 0;					
int			g_color_mode = 0;

// ������
#define		DRAG_OFF		0				
#define		DRAG_LEFT		1
#define		DRAG_RIGHT		2

int			g_lastX = -1;
int			g_lastY = -1;					
int			g_mode = 0;
int			g_dragging = 0;

#define		MODE_CAM		0
#define		MODE_CAM_TO		1
#define		MODE_OBJ		2
#define		MODE_OBJPOS		3
#define		MODE_OBJGRP		4
#define		MODE_LIGHTPOS	5
#define		MODE_DOF		6

void drawScene ( float* viewmat, bool bShade )
{
	if ( g_shade <= 1 && bShade ) {		

		glEnable ( GL_LIGHTING );
		glEnable ( GL_LIGHT0 );
		glDisable ( GL_COLOR_MATERIAL );

		Vector4DF amb, diff, spec, emission;
		//float shininess = 5.0;
		float shininess = 5.0;

		glColor3f ( 1, 1, 1 );
		glLoadIdentity ();
		glLoadMatrixf ( viewmat );

		float pos[4];
		pos[0] = g_light[0].x;
		pos[1] = g_light[0].y;
		pos[2] = g_light[0].z;
		pos[3] = 1;
		amb.Set ( 0,0,0,1 ); diff.Set ( .9,.9,.9,1 ); spec.Set(.9,.9,.9,1);
		//amb.Set ( 1.0,1.0,1.0,1 ); diff.Set ( 1,1,1,1 ); spec.Set(.1,.1,.1,1);
		glLightfv ( GL_LIGHT0, GL_POSITION, (float*) &pos[0]);
		glLightfv ( GL_LIGHT0, GL_AMBIENT, (float*) &amb.x );
		glLightfv ( GL_LIGHT0, GL_DIFFUSE, (float*) &diff.x );
		glLightfv ( GL_LIGHT0, GL_SPECULAR, (float*) &spec.x ); 

		//amb.Set ( 0,0,0,1 ); diff.Set ( .3, .3, .3, 1); spec.Set (.1,.1,.1,1);
		amb.Set ( 0,0,0,1 ); diff.Set ( .9, .9, .9, 1); spec.Set (0,0,0,1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*)&amb.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*)&diff.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&spec.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, (float*)&shininess);
		glLoadMatrixf ( viewmat );

		glBegin ( GL_QUADS );
		glNormal3f ( 0, -1, 0.001  );
		int a=0, b=0;
		float step = 25;
		float bmin = -520, bmax = 480;
		float color1, color2;
		color1 = 210. / 256.;
		float plane_y = -2;

		for (float x=bmin; x <= bmax; x += step ) {
			b = 0;
			for (float y=bmin; y <= bmax; y += step ) {
				if ((a + b) % 2 == 0)
					emission.Set(.9, .9, .9, 1);
				else
					emission.Set(color1, color1, color1, 1);
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*)&emission.x);
				glVertex3f ( x, plane_y, y );
				glVertex3f ( x+step, plane_y , y );
				glVertex3f ( x+step, plane_y, y+step );
				glVertex3f ( x, plane_y, y+step );
				b++;
			}
			a++;
		}
		//std::cout << a << " " << b << std::endl;
		glEnd ();

		glColor3f ( 0.1, 0.1, 0.2 );
		//glColor3f(1, 1, 1);
		glDisable ( GL_LIGHTING );

		amb.Set(0, 0, 0, 1);
		diff.Set(1, 1, 1, 1);
		spec.Set(0, 0, 0, 1);
		emission.Set(0, 128.*.8 / 256., 255*.8 / 256., 1);
		diff.Set(0, 128. / 256., 255 / 256., 1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*)&amb.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*)&diff.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&spec.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*)&emission.x);
		g_fluid_system.Draw ( g_camera, 0.8 );				// draw particles	

	} else {
		glDisable ( GL_LIGHTING );
		g_fluid_system.Draw ( g_camera, 0.55 );			// draw particles
	}
}

void draw2D ()
{	
	mint::Time start, stop;

	glDisable ( GL_LIGHTING );  
	glDisable ( GL_DEPTH_TEST );

	glMatrixMode ( GL_PROJECTION );
	glLoadIdentity ();  
	glScalef ( 2.0/g_window_width, -2.0/g_window_height, 1 );		// Setup view (0,0) to (800,600)
	glTranslatef ( -g_window_width/2.0, -g_window_height/2, 0.0);


	float view_matrix[16];
	glMatrixMode ( GL_MODELVIEW );
	glLoadIdentity ();

	// ������Ϣ
	if ( g_fluid_system.selected() != -1 ) {	
		g_fluid_system.DrawParticleInfo ();
		return;	
	}

	char disp[200];

	glColor4f ( 1.0, .0, .0, 1.0 );	
	strcpy ( disp, "Press H for help." );		drawText2D ( 10, 20, disp );  

	glColor4f ( .0, .0, 0.0, 1.0 );	
	strcpy ( disp, "" );
	if ( g_fluid_system.GetToggle(PCAPTURE) ) strcpy ( disp, "CAPTURING VIDEO");
	drawText2D ( 200, 20, disp );

	if ( g_help ) {	

		sprintf ( disp,	"Method (f/g):        %s", g_fluid_system.GetModeStr().c_str() );																		drawText2D ( 20, 40,  disp );
		sprintf ( disp,	"Num Particles:       %d", g_fluid_system.num_points() );																				drawText2D ( 20, 60,  disp );
		sprintf ( disp,	"Grid Resolution:     %d x %d x %d (%d)", (int) g_fluid_system.grid_res().x, (int) g_fluid_system.grid_res().y, 
			(int) g_fluid_system.grid_res().z, g_fluid_system.grid_total() );																					drawText2D ( 20, 80,  disp );
		int nsrch = pow ( g_fluid_system.grid_adj_cnt(), 1/3.0 );
		sprintf ( disp,	"Grid Search:         %d x %d x %d", nsrch, nsrch, nsrch );																				drawText2D ( 20, 100,  disp );
		
		sprintf ( disp,	"Insert Time:         %.3f ms", g_fluid_system.GetParam(PTIME_INSERT)  );																drawText2D ( 20, 140,  disp );
		sprintf ( disp,	"Sort Time:           %.3f ms", g_fluid_system.GetParam(PTIME_SORT)  );																	drawText2D ( 20, 160,  disp );
		sprintf ( disp,	"Count Time:          %.3f ms", g_fluid_system.GetParam(PTIME_COUNT)  );																drawText2D ( 20, 180,  disp );
		sprintf ( disp,	"Pressure Time:       %.3f ms", g_fluid_system.GetParam(PTIME_PRESS) );																	drawText2D ( 20, 200,  disp );
		sprintf ( disp,	"Force Time:          %.3f ms", g_fluid_system.GetParam(PTIME_FORCE) );																	drawText2D ( 20, 220,  disp );
		sprintf ( disp,	"Advance Time:        %.3f ms", g_fluid_system.GetParam(PTIME_ADVANCE) );																drawText2D ( 20, 240,  disp );
		sprintf ( disp,	"other force Time:    %.3f ms", g_fluid_system.GetParam(PTIME_OTHER_FORCE) );															drawText2D ( 20, 260,  disp );
		sprintf ( disp,	"PCI Step Time:       %.3f ms", g_fluid_system.GetParam(PTIME_PCI_STEP) );																drawText2D ( 20, 280,  disp );

		float st = 0.0f;
		switch((int)g_fluid_system.GetParam(PRUN_MODE))
		{
		case RUN_CPU_SPH:
			st = g_fluid_system.GetParam(PTIME_INSERT) + g_fluid_system.GetParam(PTIME_PRESS)+g_fluid_system.GetParam(PTIME_FORCE) + g_fluid_system.GetParam(PTIME_ADVANCE) +
				g_fluid_system.GetParam(PTIME_PCI_STEP) + g_fluid_system.GetParam(PTIME_OTHER_FORCE);
			break;
		case RUN_CUDA_INDEX_SPH:
		case RUN_CUDA_FULL_SPH:
			st = g_fluid_system.GetParam(PTIME_INSERT) + g_fluid_system.GetParam(PTIME_SORT) + g_fluid_system.GetParam(PTIME_PRESS) + g_fluid_system.GetParam(PTIME_FORCE) + g_fluid_system.GetParam(PTIME_ADVANCE);
			break;
		case RUN_CPU_PCISPH:
			st = g_fluid_system.GetParam(PTIME_INSERT) + g_fluid_system.GetParam(PTIME_OTHER_FORCE) + g_fluid_system.GetParam(PTIME_PCI_STEP) + g_fluid_system.GetParam(PTIME_ADVANCE);
			break;
		case RUN_CUDA_INDEX_PCISPH:
		case RUN_CUDA_FULL_PCISPH:
			st = g_fluid_system.GetParam(PTIME_INSERT) + g_fluid_system.GetParam(PTIME_SORT) + g_fluid_system.GetParam(PTIME_OTHER_FORCE) + g_fluid_system.GetParam(PTIME_PCI_STEP) + g_fluid_system.GetParam(PTIME_ADVANCE);
			break;
		default:
			st = g_fluid_system.GetParam(PTIME_INSERT) + g_fluid_system.GetParam(PTIME_PRESS)+g_fluid_system.GetParam(PTIME_FORCE) + g_fluid_system.GetParam(PTIME_ADVANCE);
			break;
		}	

		sprintf ( disp,	"Total Sim Time:      %.3f ms, %.1f fps", st, 1000.0/st );																				 drawText2D ( 20, 320,  disp );

		Vector3DF vol = g_fluid_system.GetVec(PGRIDVOLUMEMAX);
		vol -= g_fluid_system.GetVec(PGRIDVOLUMEMIN);
		sprintf ( disp,	"Time Step (dt):        %3.5f", g_fluid_system.GetDT () );					drawText2D ( 20, 360,  disp );
		sprintf ( disp,	"Simulation Scale:      %3.5f", g_fluid_system.GetParam(PSIMSCALE) );		drawText2D ( 20, 380,  disp );
		sprintf ( disp,	"Smooth Radius (m):     %3.5f", g_fluid_system.GetParam(PSMOOTHRADIUS) );	drawText2D ( 20, 400,  disp );
		sprintf ( disp,	"Particle Radius (m):   %3.5f", g_fluid_system.GetParam(PCOLLISIONRADIUS) );drawText2D ( 20, 420,  disp );
		sprintf ( disp,	"Particle Mass (kg):    %0.5f", g_fluid_system.GetParam(PMASS) );			drawText2D ( 20, 440,  disp );
		sprintf ( disp,	"Rest Density (kg/m^3): %3.5f", g_fluid_system.GetParam(PRESTDENSITY) );	drawText2D ( 20, 460,  disp );
		sprintf ( disp,	"Viscosity:             %3.5f", g_fluid_system.GetParam(PVISC) );			drawText2D ( 20, 480,  disp );
		sprintf ( disp,	"Boundary Stiffness:    %3.5f", g_fluid_system.GetParam(PBOUNDARYSTIFF) );	drawText2D ( 20, 500,  disp );
		sprintf ( disp,	"Boundary Dampening:    %4.5f", g_fluid_system.GetParam(PBOUNDARYDAMP) );	drawText2D ( 20, 520,  disp );

		vol = g_fluid_system.GetVec ( PPLANE_GRAV_DIR );
		sprintf ( disp,	"Gravity:               (%3.2f, %3.2f, %3.2f)", vol.x, vol.y, vol.z );		drawText2D ( 20, 540,  disp );
	}
}

void display () 
{
	mint::Time tstart, tstop;	
	mint::Time rstart, rstop;	

	tstart.SetSystemTime ( ACC_NSEC );

	// ��ʼģ��
	if ( ! g_fluid_system.GetToggle(PPAUSE)) 
		g_fluid_system.Run(g_window_width, g_window_height);

	// ����֡��
	measureFPS ();

	glEnable ( GL_DEPTH_TEST );

	rstart.SetSystemTime ( ACC_NSEC );
	disableShadows ();

	// ���֡����
	if ( g_shade<=1 ) 	
		glClearColor( 0.1, 0.1, 0.1, 1.0 );
	else				
		glClearColor ( 0, 0, 0, 0 );

	glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glDisable ( GL_CULL_FACE );
	glShadeModel ( GL_SMOOTH );

	// ���������
	g_camera.updateMatrices ();
	glMatrixMode ( GL_PROJECTION );
	glLoadMatrixf ( g_camera.getProjMatrix().GetDataF() );

	// ������ά����	
	glEnable ( GL_LIGHTING );  
	glMatrixMode ( GL_MODELVIEW );
	glLoadMatrixf ( g_camera.getViewMatrix().GetDataF() );
	drawScene ( g_camera.getViewMatrix().GetDataF() , true );

	// ���ƶ�ά������Ϣ
	draw2D ();

	if ( g_fluid_system.GetToggle(PPROFILE) ) 
	{ 
		rstop.SetSystemTime ( ACC_NSEC ); 
		rstop = rstop - rstart; 
		printf ( "RENDER: %s\n", rstop.GetReadableTime().c_str() );
	}

	// ����������
	glutSwapBuffers();  
	glutPostRedisplay();

	if ( g_fluid_system.GetToggle(PPROFILE) ) { 
		tstop.SetSystemTime ( ACC_NSEC ); tstop = tstop - tstart; 
		printf ( "TOTAL:  %s, %f fps\n", tstop.GetReadableTime().c_str(), 1000.0/tstop.GetMSec() ); 
		printf ( "PERFORMANCE:  %d particles/sec, %d\n", (int) (g_fluid_system.num_points() * 1000.0)/tstop.GetMSec(), g_fluid_system.num_points() ); 
	}
}

void reshape ( int width, int height ) 
{
	// �����Ӵ��ĸ߶�&���
	g_window_width  = (float) width;
	g_window_height = (float) height;
	glViewport( 0, 0, width, height );  
}

void UpdateEmit ()
{	
	g_obj_from = g_fluid_system.GetVec ( PEMIT_POS );
	g_obj_angles = g_fluid_system.GetVec ( PEMIT_ANG );
	g_obj_dang = g_fluid_system.GetVec ( PEMIT_RATE );
}

void keyboard_func ( unsigned char key, int x, int y )
{
	Vector3DF fp = g_camera.getPos ();
	Vector3DF tp = g_camera.getToPos ();

	switch( key ) {
		// ��ͣ
	case ' ':				
		g_fluid_system.Toggle(PPAUSE);
		break;	
		// ��һ��ģ�ⷽ��
	case 'B':
	case 'b':
		// �л�����3Dģ������/�ֶ����ó���
		g_fluid_system.Toggle(PUSELOADEDSCENE);
		g_fluid_system.Setup (false);
		break;
	case 'f': 
	case 'F':		
		g_fluid_system.IncParam ( PRUN_MODE, -1, 0, 5 );		
		g_fluid_system.Setup (false); 
		break;
		// ��һ��ģ�ⷽ��
	case 'g': 
	case 'G':		
		g_fluid_system.IncParam ( PRUN_MODE, 1, 0, 2 );		
		g_fluid_system.Setup (false); 
		break;
		// ��ʾ/���ؾ�������߽�
	case '1':				
		g_fluid_system.Toggle(PDRAWGRIDBOUND);
		break;		
		// ��ʾ/���������߽�
	case '2':				
		g_fluid_system.Toggle(PDRAWDOMAIN);
		break;	
		// ��ʾ/���ؾ�������
	case '3':
		g_fluid_system.Toggle(PDRAWGRIDCELLS);
		break;
		// �ı�������ƶ�ģʽ->ƽ��
	case 'C':	
		g_mode = MODE_CAM_TO;	
		break;
		// �ı�������ƶ�ģʽ->��ת
	case 'c': 	
		g_mode = MODE_CAM;	
		break; 
		// ��ʾ������Ϣ
	case 'h': 
	case 'H':	
		g_help = !g_help; 
		break;  
		// �л���Դλ�ÿ���ģʽ
	case 'l': 
	case 'L':	
		g_mode = MODE_LIGHTPOS;	
		break;
		// �л�����������ʾģʽ
	case 'j': 
	case 'J': 
		{
			int d = g_fluid_system.GetParam ( PDRAWMODE ) + 1;
			if ( d > 2 ) d = 0;
			g_fluid_system.SetParam ( PDRAWMODE, d );
		}
		break;	
		// �ƶ����/���Ӿ�Ч���ϵ�ͬ���ƶ����壩
	case 'a': 
	case 'A':		
		g_camera.setToPos( tp.x - 1, tp.y, tp.z ); 
		break;
	case 'd': 
	case 'D':		
		g_camera.setToPos( tp.x + 1, tp.y, tp.z ); 
		break;
	case 'w': 
	case 'W':		
		g_camera.setToPos( tp.x, tp.y - 1, tp.z ); 
		break;
	case 's': 
	case 'S':		
		g_camera.setToPos( tp.x, tp.y + 1, tp.z ); 
		break;
	case 'q': 
	case 'Q':		
		g_camera.setToPos( tp.x, tp.y, tp.z + 1 ); 
		break;
	case 'z': 
	case 'Z':		
		g_camera.setToPos( tp.x, tp.y, tp.z - 1 );
		break;
		// ��һ������
	case '[':
		g_fluid_system.IncParam ( PEXAMPLE, -1, 0, 3 );
		g_fluid_system.Setup (true);
		UpdateEmit ();
		break;
		// ��һ������
	case ']':
		g_fluid_system.IncParam ( PEXAMPLE, +1, 0, 3 );
		g_fluid_system.Setup (true);
		UpdateEmit ();
		break;  
	case 27:			    
		exit( 0 ); 
		break;
	default:
		break;
	}
}

void mouse_click_func ( int button, int state, int x, int y )
{
	if( state == GLUT_DOWN ) {
		if ( button == GLUT_LEFT_BUTTON )		
			g_dragging = DRAG_LEFT;
		else if ( button == GLUT_RIGHT_BUTTON ) 
			g_dragging = DRAG_RIGHT;	
		g_lastX = x;
		g_lastY = y;	
	} else if ( state==GLUT_UP ) {
		g_dragging = DRAG_OFF;
	}
}

void mouse_move_func ( int x, int y )
{
	g_fluid_system.SelectParticle ( x, y, g_window_width, g_window_height, g_camera );
}

void mouse_drag_func ( int x, int y )
{
	int dx = x - g_lastX;
	int dy = y - g_lastY;

	switch ( g_mode ) {
	case MODE_CAM:
		if ( g_dragging == DRAG_LEFT ) {
			g_camera.moveOrbit ( dx, dy, 0, 0 );
		} else if ( g_dragging == DRAG_RIGHT ) {
			g_camera.moveOrbit ( 0, 0, 0, dy*0.15 );
		} 
		break;
	case MODE_CAM_TO:
		if ( g_dragging == DRAG_LEFT ) {
			g_camera.moveToPos ( dx*0.1, 0, dy*0.1 );
		} else if ( g_dragging == DRAG_RIGHT ) {
			g_camera.moveToPos ( 0, dy*0.1, 0 );
		}
		break;	
	case MODE_OBJ:
		if ( g_dragging == DRAG_LEFT ) {
			g_obj_angles.x -= dx*0.1;
			g_obj_angles.y += dy*0.1;
			printf ( "Obj Angs:  %f %f %f\n", g_obj_angles.x, g_obj_angles.y, g_obj_angles.z );
		} else if (g_dragging == DRAG_RIGHT) {
			g_obj_angles.z -= dy*.005;			
			printf ( "Obj Angs:  %f %f %f\n", g_obj_angles.x, g_obj_angles.y, g_obj_angles.z );
		}
		g_fluid_system.SetVec ( PEMIT_ANG, Vector3DF ( g_obj_angles.x, g_obj_angles.y, g_obj_angles.z ) );
		break;
	case MODE_OBJPOS:
		if ( g_dragging == DRAG_LEFT ) {
			g_obj_from.x -= dx*.1;
			g_obj_from.y += dy*.1;
			printf ( "Obj:  %f %f %f\n", g_obj_from.x, g_obj_from.y, g_obj_from.z );
		} else if (g_dragging == DRAG_RIGHT) {
			g_obj_from.z -= dy*.1;
			printf ( "Obj:  %f %f %f\n", g_obj_from.x, g_obj_from.y, g_obj_from.z );
		}
		g_fluid_system.SetVec ( PEMIT_POS, Vector3DF ( g_obj_from.x, g_obj_from.y, g_obj_from.z ) );
		break;
	case MODE_LIGHTPOS:
		if ( g_dragging == DRAG_LEFT ) {
			g_light[0].x -= dx*.1;
			g_light[0].z -= dy*.1;		
			printf ( "Light: %f %f %f\n", g_light[0].x, g_light[0].y, g_light[0].z );
		} else if (g_dragging == DRAG_RIGHT) {
			g_light[0].y -= dy*.1;			
			printf ( "Light: %f %f %f\n", g_light[0].x, g_light[0].y, g_light[0].z );
		}	
		break;
	}

	if ( x < 10 || y < 10 || x > 1000 || y > 700 ) {
		glutWarpPointer ( 1024/2, 768/2 );
		g_lastX = 1024/2;
		g_lastY = 768/2;
	} else {
		g_lastX = x;
		g_lastY = y;
	}
}

void idle_func ()
{
}

void glInit ()
{
	// �򿪿���ݣ�����ʾ�����ܽ�����ѵĴ���
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH); // �Ե����ƽ������
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH); // ��ֱ�߽���ƽ������
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POLYGON_SMOOTH); // �Զ���ν���ƽ������
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	srand ( time ( 0x0 ) );

	glClearColor( 0.49, 0.49, 0.49, 1.0 );
	glShadeModel( GL_SMOOTH );

	glEnable ( GL_COLOR_MATERIAL ); // ������ɫ׷��
	glEnable (GL_DEPTH_TEST);  
	glEnable ( GL_TEXTURE_2D ); //����2D����ӳ��

	// �ص�����
	glutDisplayFunc( display );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard_func );
	glutMouseFunc( mouse_click_func );  
	glutMotionFunc( mouse_drag_func );
	glutIdleFunc( idle_func );

	// ��ʼ�������
	g_camera.setOrbit ( Vector3DF(260,15,50), Vector3DF(0,50,0), 300, 400 );
	g_camera.setFov ( 35 );
	g_camera.updateMatrices ();

	g_light[0].x = 0;		g_light[0].y = 200;	g_light[0].z = 0; g_light[0].w = 1;
	g_light_to[0].x = 0;	g_light_to[0].y = 0;	g_light_to[0].z = 0; g_light_to[0].w = 1;

	g_light[1].x = 55;		g_light[1].y = 140;	g_light[1].z = 50;	g_light[1].w = 1;
	g_light_to[1].x = 0;	g_light_to[1].y = 0;	g_light_to[1].z = 0;		g_light_to[1].w = 1;

	g_light_fov = 45.0f;

	g_obj_from.x = 0;		g_obj_from.y = 0;		g_obj_from.z = 20;		// emitter
	g_obj_angles.x = 118.7;	g_obj_angles.y = 200;	g_obj_angles.z = 1.0;
	g_obj_dang.x = 1;	g_obj_dang.y = 1;		g_obj_dang.z = 0;

	g_fluid_system.Setup (true);
	g_fluid_system.SetVec ( PEMIT_ANG, Vector3DF ( g_obj_angles.x, g_obj_angles.y, g_obj_angles.z ) );
	g_fluid_system.SetVec ( PEMIT_POS, Vector3DF ( g_obj_from.x, g_obj_from.y, g_obj_from.z ) );

	g_fluid_system.SetParam ( PCLR_MODE, g_color_mode );	
}

int main ( int argc, char **argv )
{
#ifdef USE_CUDA
	// ��ʼ��CUDA
	cudaInit( argc, argv );
#endif

	// GLUT�������
	glutInit( &argc, &argv[0] ); 
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( (int) g_window_width, (int) g_window_height );
	glutCreateWindow ( "SPH" );

	// OPENGL��ʼ������
	glInit();	

	g_fluid_system.SetupRender ();
	glutMainLoop();

	g_fluid_system.ExitParticleSystem ();

	return 0;
}

