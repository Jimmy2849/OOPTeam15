////////////////////////////////////////////////////////////////////////////////
// 
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////
// curling , player 객체 추가
// setup에서 공 생성부분은 Curling 객체 생성시 수행, Curing객체 game을 전역으로 선언, display에서도 이와 맞게 교체, VK_SPACE 부분 g_sphere[i] 를 game.getPlayer().getball(i)로 교체.
// display 함수에서 game 의 now_turn 을 확인하여 턴이 종료되었으면( 스페이스바를 눌러 now_turn++ 이 되고 공이 모두 멈추었으면) 그에 맞게 세팅한다.
// 또한 모든 턴이 종료되었으면 game.nextSet()를 호출하여 다음 세트를 진행한다.
// CSphere 에 bool isPlaying, void setIsPlaying() 추가. 이 공이 player 가 굴려서 게임에 참여된 공인지 판단. 
// 먼저 1p 부터 시작하고 그다음 2p 가 공을 굴린다. Curling.whose_trun 이 현재 player를 나타내며, 굴릴때마다 값이 바뀐다.
// CText 는 https://github.com/kyung2/cau_OOP02 에서 가져옴, p1와 p2의 점수 화면에 출력.
// **게임에 참여중인 공이 출발지점에 있으면 간섭생김..
// 다음 세트로 넘어갈때 점수가 Curling.total_score에 합산. 이를 상단에 출력 ( 텍스트 형식은 수정 가능 )
// 모든 세트가 끝나면 승자 결과 출력. 다음 게임 구현 x
// 진짜 컬링처럼 점수 계산 - 기존 방식이 좋을지?
// 세트마다 선 플레이어 교체 - 짝수 세트 시 2플레이어 부터

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <string>
#include "CText.h"
#include <algorithm> // Culring class에서 점수 계산시 sort 사용

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[4][2] = { {-2.8f,-4.2f} , {-2.3f,-4.2f} , {-1.8f, -4.2f} , {-1.3f,-4.2f} };
const float spherePos2[4][2] = { {+2.8f,-4.2f} , {+2.3f,-4.2f} , {+1.8f, -4.2f} , {+1.3f,-4.2f} };
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[4] = {d3d::RED, d3d::RED, d3d::YELLOW, d3d::WHITE};

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;


#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

D3DXVECTOR3 scoreTarget = { 0.0f, (float)M_RADIUS, 0.0f }; // 점수 기준이 되는 기준점 (화면상 하얀 점 보이는 중앙)

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private :
	float					center_x, center_y, center_z;
    float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;
	int						score;
	bool					isPlaying;
	float					distance;

public:
    CSphere(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		score = 1;
		isPlaying = false;
		distance = 99;
        m_pSphereMesh = NULL;
    }
    ~CSphere(void) {}

public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;
		
        m_mtrl.Ambient  = color;
        m_mtrl.Diffuse  = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power    = 5.0f;
		
        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }
	
    void destroy(void)
    {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
    }
	void setDistance(void) {
		distance = sqrt(pow((scoreTarget.x - this->center_x), 2) + pow((scoreTarget.z - this->center_z), 2));
	}
	void setScore(void) { // 공 점수 계산
		float distance = sqrt(pow((scoreTarget.x - this->center_x), 2) + pow((scoreTarget.z - this->center_z), 2));
		int n = 30 - 10 * (int)distance;
		if (n > 0) {
			this->score = n;
		}
		else {
			this->score = 0; // 점수가 음수면 안 되므로 음수가 나왔을 경우 0으로 저장
		}
	}

	bool isStop(void) // 공 멈춤 판정
	{
		if (this->m_velocity_x == 0 && this->m_velocity_z == 0) {
			return true;
		}
		else {
			return false;
		}
	}
	bool hasIntersected(CSphere& ball) // 공 충돌 판정
	{
		D3DXVECTOR3 b1 = this->getCenter();
		D3DXVECTOR3 b2 = ball.getCenter();

		double distance = sqrt(pow((b1.x - b2.x), 2) + pow((b1.y - b2.y), 2) + pow((b1.z - b2.z), 2)); // 공의 중심 좌표를 통해 구한 두 공 사이의 거리
		double rSum = this->getRadius() + ball.getRadius(); // 두 공의 반지름의 합

		// 두 공 사이의 거리가 두 공의 반지름의 합보다 작거나 같으면 두 공은 충돌한 것 && 두 공의 isPlaying이 모두 true여야 충돌 조건을 만족
		if (distance <= rSum && this->isPlaying && ball.isPlaying) {
			return true;
		}
		else {
			return false;
		}

	}
	
	void hitBy(CSphere& ball) // 공 충돌 처리
	{ 
		// declare variable, for performance I set them as static
		/*static D3DXVECTOR3 direction;
		static D3DXVECTOR3 warpVector;
		static D3DXVECTOR3 totalVelocity;
		static D3DXVECTOR3 normalizedDirection;
		static D3DXVECTOR3 ballVelocity;
		static D3DXVECTOR3 thisVelocity;
		static const float fix = 1.1f; // for correction of should-be-warp(겹쳐지지 않게 옮겨져야 되는) distance //should be larger then one //1보다 커야함
		static float distance;
		static float overlapInterval;

		// when collided, do physics
		if (hasIntersected(ball)) {
			// set direction
			direction = this->getCenter() - ball.getCenter();
			// compute distance // 2 차원이라고 가정
			distance = sqrt(direction.x * direction.x + direction.z * direction.z);
			// overlap distance
			overlapInterval = 2 * ball.getRadius() - distance;
			// how much should I warp so that circles(colliders) won't overlapped anymore
			warpVector = fix * direction * (overlapInterval / (2 * ball.getRadius() - overlapInterval));

			// implementation of collision // 탄성 충돌 구현 
			if (((ball.m_velocity_x * ball.m_velocity_x) + (ball.m_velocity_z * ball.m_velocity_z)) >= ((this->m_velocity_x * this->m_velocity_x) + (this->m_velocity_z * this->m_velocity_z))) {
				// hitter의 속도가 큰 경우 반대로 호출 // 왜냐면 아래의 물리식은 hitee의 기준으로 만들었기 때문
				ball.hitBy(*this);
				return;
			}
			else {
				// hitter의 속도가 작은 경우
				// 충돌할 때, 워프시켜서 한번만 아래의 물리연산이 실행되도록 하기 
				// this에 +warpVector 적용
				this->setCenter(this->getCenter().x + warpVector.x, this->getCenter().y, this->getCenter().z + warpVector.z);
			}

			// Add all velocity of colliding Balls
			totalVelocity = D3DXVECTOR3(getVelocity_X() + ball.getVelocity_X(), 0, getVelocity_Z() + ball.getVelocity_Z());
			// normalize direction vector
			normalizedDirection = (-1) * direction / distance;

			// compute final velocity of each colliders
			ballVelocity = normalizedDirection * (normalizedDirection.x * totalVelocity.x + normalizedDirection.z * totalVelocity.z);
			thisVelocity = -ballVelocity + totalVelocity;

			// set Power // 탄성 충돌 물리식 결과 적용
			this->setPower(thisVelocity.x, thisVelocity.z);
			ball.setPower(ballVelocity.x, ballVelocity.z);
			*/
		if (hasIntersected(ball)) {

			double x = this->center_x - ball.center_x; // 두 공 사이의 x좌표 거리
			double z = this->center_z - ball.center_z; // 두 공 사이의 z좌표 거리
			double distance = sqrt((x * x) + (z * z)); // 두 공 사이의 직선 거리

			// this의 충돌 전 속도
			double vax = this->m_velocity_x;
			double vaz = this->m_velocity_z;

			// ball의 충돌 전 속도
			double vbx = ball.m_velocity_x;
			double vbz = ball.m_velocity_z;

			// θ = 수평 방향과 두 공의 중심을 이은 직선이 이루는 각
			double cos_t = x / distance; // cos θ
			double sin_t = z / distance; // sin θ

			// 충돌 후 두 공의 x'방향 속도
			double vaxp = vbx * cos_t + vbz * sin_t;
			double vbxp = vax * cos_t + vaz * sin_t;

			// 충돌 후 두 공의 z'방향 속도
			double vazp = vaz * cos_t - vax * sin_t;
			double vbzp = vbz * cos_t - vbx * sin_t;

			// 충돌 후 this의 속도
			double vaxp2 = vaxp * cos_t - vazp * sin_t;
			double vazp2 = vaxp * sin_t + vazp * cos_t;

			// 충돌 후 ball의 속도
			double vbxp2 = vbxp * cos_t - vbzp * sin_t;
			double vbzp2 = vbxp * sin_t + vbzp * cos_t;

			this->setPower(vaxp2, vazp2);
			ball.setPower(vbxp2, vbzp2);

		}
	}

	void ballUpdate(float timeDiff) 
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		if(vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE*timeDiff*m_velocity_x;
			float tZ = cord.z + TIME_SCALE*timeDiff*m_velocity_z;

			//correction of position of ball
			// Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
			/*if(tX >= (4.5 - M_RADIUS))
				tX = 4.5 - M_RADIUS;
			else if(tX <=(-4.5 + M_RADIUS))
				tX = -4.5 + M_RADIUS;
			else if(tZ <= (-3 + M_RADIUS))
				tZ = -3 + M_RADIUS;
			else if(tZ >= (3 - M_RADIUS))
				tZ = 3 - M_RADIUS;*/
			
			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0,0);}
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		double rate = 1 -  (1 - DECREASE_RATE)*timeDiff * 400;
		if(rate < 0 )
			rate = 0;
		this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
	}

	double getVelocity_X() { return this->m_velocity_x;	}
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x=x;	center_y=y;	center_z=z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}
	
	float getRadius(void)  const { return (float)(M_RADIUS);  }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const
    {
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }
	int getScore() { return score; }
	float getDistance() { return distance; }
	void setIsPlaying(bool b) {
		isPlaying = b;
	}

private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh*              m_pSphereMesh;
	
};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:
	
    float					m_x;
	float					m_z;
	float                   m_width;
    float                   m_depth;
	float					m_height;
	
public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;
		
        m_mtrl.Ambient  = color;
        m_mtrl.Diffuse  = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power    = 5.0f;
		
        m_width = iwidth;
        m_depth = idepth;
		
        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
    }
	
	bool hasIntersected(CSphere& ball) 
	{
		/*float ballpos_x = ball.getCenter().x;
		float ballpos_z = ball.getCenter().z;

		// for better collision detection // 벽을 좀 작게해서 보정
		float correction = 0.08f;

		// compare position with wall and ball position
		if ((ballpos_x >= ((3.0 - correction) - M_RADIUS))
			|| (ballpos_x <= ((-1) * (3.0 - correction) + M_RADIUS))
			|| (ballpos_z <= ((-1) * (4.5 - correction) + M_RADIUS))
			|| (ballpos_z >= (4.5 - correction - M_RADIUS)))
		{
			return true;
		}
		else {
			return false;
		}*/

		float ball_x = ball.getCenter().x;
		float ball_z = ball.getCenter().z;

		// 공의 x or z좌표가 벽의 좌표와 같거나 넘어설 경우 충돌 판정 
		if ((ball_x >= (3.0 - M_RADIUS)) || (ball_x <= (-3.0 + M_RADIUS)) || (ball_z <= (-4.5 + M_RADIUS)) || (ball_z >= (4.5 - M_RADIUS))) {
			return true;
		}
		else {
			return false;
		}
	}

	void hitBy(CSphere& ball) 
	{
		// when collided, do physics
		/*if (hasIntersected(ball)) {
			static const float energyComsumption = 0.2f;
			// Collide with Upper Wall // 상위의 벽과 충돌
			if (ball.getCenter().z >= (4.5 - M_RADIUS)) {
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, 4.5 - M_RADIUS);
				ball.setPower(ball.getVelocity_X(), (-1) * ball.getVelocity_Z());

				// decrease velocity of ball after collision of wall
				ball.setPower(ball.getVelocity_X() * (1.0f - energyComsumption), ball.getVelocity_Z() * (1.0f - energyComsumption));
			}
			// Collide with Lower Wall // 하위의 벽과 충돌
			if (ball.getCenter().z <= (-(4.5) + M_RADIUS)) {
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, -4.5 + M_RADIUS);
				ball.setPower(ball.getVelocity_X(), (-1) * ball.getVelocity_Z());

				// decrease velocity of ball after collision of wall
				ball.setPower(ball.getVelocity_X() * (1.0f - energyComsumption), ball.getVelocity_Z() * (1.0f - energyComsumption));
			}
			// Collide with Left Wall // 좌측의 벽과 충돌
			if (ball.getCenter().x <= (-(3.0) + M_RADIUS))
			{
				ball.setCenter(-3.0 + M_RADIUS, ball.getCenter().y, ball.getCenter().z);
				ball.setPower((-1) * ball.getVelocity_X(), ball.getVelocity_Z());

				// decrease velocity of ball after collision of wall
				ball.setPower(ball.getVelocity_X() * (1.0f - energyComsumption), ball.getVelocity_Z() * (1.0f - energyComsumption));
			}
			// Collide with Right Wall // 우측의 벽과 충돌
			if (ball.getCenter().x >= (3.0 - M_RADIUS)) {
				ball.setCenter(3.0 - M_RADIUS, ball.getCenter().y, ball.getCenter().z);
				ball.setPower((-1) * ball.getVelocity_X(), ball.getVelocity_Z());

				// decrease velocity of ball after collision of wall
				ball.setPower(ball.getVelocity_X() * (1.0f - energyComsumption), ball.getVelocity_Z() * (1.0f - energyComsumption));
			}
		}*/
		if (hasIntersected(ball)) {

			// 왼쪽 벽과 충돌했을 경우
			if (ball.getCenter().x <= (-3.0 + M_RADIUS)) {
				ball.setCenter(-3.0 + M_RADIUS, ball.getCenter().y, ball.getCenter().z); // 공 좌표 다시 설정
				ball.setPower((-1) * ball.getVelocity_X(), ball.getVelocity_Z()); // x방향 속도를 반대 방향으로 바꿈
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // 벽 부딪힌 후 감속 처리
			}

			// 오른쪽 벽과 충돌했을 경우
			if (ball.getCenter().x >= (3.0 - M_RADIUS)) {
				ball.setCenter(3.0 - M_RADIUS, ball.getCenter().y, ball.getCenter().z); // 공 좌표 다시 설정
				ball.setPower((-1) * ball.getVelocity_X(), ball.getVelocity_Z()); // x방향 속도를 반대 방향으로 바꿈
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // 벽 부딪힌 후 감속 처리
			}

			// 위쪽 벽과 충돌했을 경우
			if (ball.getCenter().z >= (4.5 - M_RADIUS)) {
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, 4.5 - M_RADIUS); // 공 좌표 다시 설정
				ball.setPower(ball.getVelocity_X(), (-1) * ball.getVelocity_Z()); // z방향 속도를 반대 방향으로 바꿈
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // 벽 부딪힌 후 감속 처리
			}

			// 아래쪽 벽과 충돌했을 경우
			if (ball.getCenter().z <= (-4.5 + M_RADIUS)) {
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, -4.5 + M_RADIUS); // 공 좌표 다시 설정
				ball.setPower(ball.getVelocity_X(), (-1) * ball.getVelocity_Z()); // z방향 속도를 반대 방향으로 바꿈
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // 벽 부딪힌 후 감속 처리
			}
		}

	}    
	
	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}
	
    float getHeight(void) const { return M_HEIGHT; }
	
	
	
private :
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	
	D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh*              m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;
		
        m_bound._center = lit.Position;
        m_bound._radius = radius;
		
        m_lit.Type          = lit.Type;
        m_lit.Diffuse       = lit.Diffuse;
        m_lit.Specular      = lit.Specular;
        m_lit.Ambient       = lit.Ambient;
        m_lit.Position      = lit.Position;
        m_lit.Direction     = lit.Direction;
        m_lit.Range         = lit.Range;
        m_lit.Falloff       = lit.Falloff;
        m_lit.Attenuation0  = lit.Attenuation0;
        m_lit.Attenuation1  = lit.Attenuation1;
        m_lit.Attenuation2  = lit.Attenuation2;
        m_lit.Theta         = lit.Theta;
        m_lit.Phi           = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;
		
        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;
		
        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh*          m_pMesh;
    d3d::BoundingSphere m_bound;
};


class Player {
private :
	CSphere ball[4]; // 한 플레이어가 가진 공
	int playerNum;
	int score;
public :
	Player() {
		playerNum = 1;
		score = 0;
	}
	Player(int n) {
		playerNum = n;
		score = 0;
	}

	bool createBalls() {
		int i = 0;
		D3DCOLOR color;
		if (playerNum == 1) color = d3d::RED;
		else if (playerNum == 2) color = d3d::YELLOW;
		for (i = 0; i < 4; i++) {
			if (false == ball[i].create(Device, color)) return false;
		}
		setBalls();
		return true;
	}
	void setBalls() {
		int i = 0;
		if (playerNum == 1) {
			for (i = 0; i < 4; i++) {
				this->ball[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
				this->ball[i].setPower(0, 0);
			}
		}
		else if (playerNum == 2) {
			for (i = 0; i < 4; i++) {
				this->ball[i].setCenter(spherePos2[i][0], (float)M_RADIUS, spherePos2[i][1]);
				this->ball[i].setPower(0, 0);
			}
		}
	}
	bool isAllStop() {
		int i = 0;
		for (i = 0; i < 4; i++) {
			if (!ball[i].isStop()) return false;
		}
		return true;
	}
	CSphere& getBall(int i) {
		return this->ball[i]; }

	void setScore() {
		int i, n = 0;
		for (i = 0; i < 4; i++) {
			this->ball[i].setScore();
			n += this->ball[i].getScore();
		}
		score = n;
	}
	void setScore(int i) {
		score = i;
	}
	int getScore() { 
		return score; }
	// distroy 추가?
};


class Curling {
private:
	Player p[2];
	int max_turn; // 최대 턴 수 ( player의 공의 개수랑 같아야 함)
	int max_set; // 최대 세트 수
	int now_set; // 지금 몇번째 세트인가
	int now_turn; // 지금 몇번째 턴인가
	int total_score[2]; // 총점
	int whose_turn; // 지금 어느 player의 턴인가 0 이면 1p 1이면 2p
public:
	Curling() {
		max_turn = 4;
		max_set = 2;
		now_set = 1;
		now_turn = 1;
		total_score[0] = 0;
		total_score[1] = 0;
		whose_turn = 0;
		p[0] = Player(1);
		p[1] = Player(2);
	}
	void scoreCheck() {
		total_score[0] += p[0].getScore();
		total_score[1] += p[1].getScore();
	}
	std::string winnerCheck() {
		std::string str;
		if (total_score[0] > total_score[1])
			str = "Player 1 Win!!";
		else if (total_score[0] < total_score[1])
			str = "Player 2 Win!!";
		else 
			str = "Draw!!";

		return str;
		
	}
	void nextSet() { // 모든 턴이 종료되면 호출되어 다음 세트를 진행하는 함수. 
		scoreCheck(); // 점수체크
		Sleep(2000); // 세트 결과 2초동안 유지(유저가 결과를 확인하는 시간)
		p[0].setBalls(); // 공 위치 초기화
		p[1].setBalls();
		now_turn = 1; // 다시 첫번째 턴으로
		now_set++; // 현재 세트는 1 증가
		whose_turn = now_set % 2 == 1 ? 0 : 1;
	}
	bool isAllStop() {
		if (p[0].isAllStop() && p[1].isAllStop()) return true;
		else return false;
	}
	void setScoreC() { // 컬링처럼 점수를 측정하는 함수. 
		float p1[4] = {99,99,99,99 };
		float p2[4] = {99,99,99,99 };
		int score_p1 = 0;
		int score_p2 = 0;
		for (int i = 0; i < getMaxTurn() ; i++) {
			getPlayer(0).getBall(i).setDistance();
			getPlayer(1).getBall(i).setDistance();
			p1[i] = getPlayer(0).getBall(i).getDistance();
			p2[i] = getPlayer(1).getBall(i).getDistance();
		}
		std::sort(p1, p1 + 4);
		std::sort(p2, p2 + 4);
		if (p1[0] < p2[0]) {
			score_p1++;
			for (int i = 1; i < getMaxTurn(); i++) {
				if (p1[i] < p2[0]) {
					score_p1++;
				}
				else break;
			}
		}
		else if (p1[0] > p2[0]) {
			score_p2++;
			for (int i = 1; i < getMaxTurn(); i++) {
				if (p2[i] < p1[0]) {
					score_p2++;
				}
				else break;
				}
		}
		getPlayer(0).setScore(score_p1);
		getPlayer(1).setScore(score_p2);

	}
	void nextTurn() {
		if (now_set % 2 == 1) {
			if (whose_turn == 1) now_turn++; // 2p의 차례가 끝났다면 다음 턴으로 이동
			changePlayer(); // 차례가 끝나면 플레이어 교체
		}
		else {
			if (whose_turn == 0) now_turn++; // 2p의 차례가 끝났다면 다음 턴으로 이동
			changePlayer(); // 차례가 끝나면 플레이어 교체
		}
	}
	Player& getPlayer(int num) { return p[num]; }
	int getMaxTurn() { return max_turn; }
	int getMaxSet() { return max_set; }
	int getNowTurn() { return now_turn; }
	int getNowSet() { return now_set; }
	int getWhoseTurn() { return whose_turn; }
	int getScore(int i) { return total_score[i]; }
	void setNowTurn(int i) { this->now_turn = i; }
	void changePlayer() { // 현제 플레이어를 바꾼다
		whose_turn = whose_turn == 0 ? 1 : 0;
	}


};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[4];
CSphere	g_sphere[4];
CSphere	g_target_blueball;
CLight	g_light;
CText   g_score; // 화면 상단 가운데 score 표시
CText	g_set; // score 바로 아래에 세트 표시
CText   g_player1; // 화면 상단 좌측에 P1의 현재 세트 점수표시
CText   g_player2; // 화면 상단 우측에 P2의 현재 세트 점수표시
CText	g_totalP1; // 화면 상단 좌측에 P1의 총점표시
CText	g_totalP2; // 화면 상단 우측에 P2의 총점표시
CText	winner; // 모든 세트 끝나면 승자 표시


//const std::string player1Str = "Player 1";
//const std::string player2Str = "Player 2";
//const std::string setStr = "SET";

Curling game;
double g_camera_pos[3] = {0.0, 5.0, -8.0};

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	//setup text
	if (g_score.create(Device, Width, Height, "SCORE") == false) return false;
	if (g_player1.create(Device, Width, Height, "Player1") == false) return false;
	if (g_player2.create(Device, Width - 30, Height, "Player2") == false) return false;
	if (g_set.create(Device, Width, Height, "SET") == false) return false;
	if (g_totalP1.create(Device, Width, Height, "p1") == false) return false;
	if (g_totalP2.create(Device, Width - 30, Height, "p2") == false) return false;
	if (winner.create(Device, Width, Height, " ") == false) return false;
	

	g_player1.setPosition(0, 50);
	g_player2.setPosition(0, 50);
	g_set.setPosition(0, 50);
	winner.setPosition(0, 100);
	g_score.setAnchor(DT_TOP | DT_CENTER);
	g_player1.setAnchor(DT_TOP | DT_LEFT);
	g_player2.setAnchor(DT_TOP | DT_RIGHT);
	g_set.setAnchor(DT_TOP | DT_CENTER);
	g_totalP1.setAnchor(DT_TOP | DT_LEFT);
	g_totalP2.setAnchor(DT_TOP | DT_RIGHT); 
	winner.setAnchor(DT_TOP | DT_CENTER);

    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);
		
	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 9, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	
	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, 6, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 4.56f);
	if (false == g_legowall[1].create(Device, -1, -1, 6, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -4.56f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 9.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(3.06f, 0.12f, 0.0f);
	if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 9.24f, d3d::DARKRED)) return false;
	g_legowall[3].setPosition(-3.06f, 0.12f, 0.0f);

	// create four balls and set the position
	/*for (i = 0; i<4; i++) {
		if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS , spherePos[i][1]);
		g_sphere[i].setPower(0,0);
	}*/
	if(false == game.getPlayer(0).createBalls()) return false; // Curling의 멤버 Player가 가지고 있는 공을 생성 (4개)
	if(false == game.getPlayer(1).createBalls()) return false;

	
	// create blue ball for set direction
    if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
	g_target_blueball.setCenter(.0f, (float)M_RADIUS , .0f);
	
	// light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type         = D3DLIGHT_POINT;
    lit.Diffuse      = d3d::WHITE; 
	lit.Specular     = d3d::WHITE * 0.9f;
    lit.Ambient      = d3d::WHITE * 0.9f;
    lit.Position     = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range        = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;
	
	// Position and aim the camera.
	D3DXVECTOR3 pos(0.0f, 14.0f, -1.0f); // 시작 카메라 위치 
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f); // 카메라 비추는 곳
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);
	
	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);
	
    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	
	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
    g_legoPlane.destroy();
	for(int i = 0 ; i < 4; i++) {
		g_legowall[i].destroy();
	}
    destroyAllLegoBlock();
    g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i=0;
	int j = 0;
	int turn = game.getNowTurn(); // 지금 몇번째 턴인가

	if( Device )
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();
		
		// 턴 관련 조정 추가

		if (game.isAllStop()) {
			if (turn > game.getMaxTurn()) { // 현재 턴이 max를 초과했다면 다음 세트를 수행한다.
				turn = 1;
				game.nextSet();
				if (game.getNowSet() > game.getMaxSet()) {
					winner.setStr(game.winnerCheck());
				}
			}
			game.getPlayer(game.getWhoseTurn()).getBall(turn - 1).setCenter(0, (float)M_RADIUS, -4);
		}


		// update the position of each ball. during update, check whether each ball hit by walls.
		/*for (i = 0; i < 4; i++) {
			g_sphere[i].ballUpdate(timeDelta);
			for(j = 0; j < 4; j++){ g_legowall[i].hitBy(g_sphere[j]); }
		}*/
		for (i = 0; i < 4; i++) {
			game.getPlayer(0).getBall(i).ballUpdate(timeDelta);
			game.getPlayer(1).getBall(i).ballUpdate(timeDelta);
			for (j = 0; j < 4; j++) {
				g_legowall[i].hitBy(game.getPlayer(0).getBall(j)); 
				g_legowall[i].hitBy(game.getPlayer(1).getBall(j));
			}
		}

		// check whether any two balls hit together and update the direction of balls
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (i >= j) { continue; }
				game.getPlayer(0).getBall(i).hitBy(game.getPlayer(0).getBall(j)); // P1 의 공끼리 체크
				game.getPlayer(1).getBall(i).hitBy(game.getPlayer(1).getBall(j)); // P2 의 공끼리 체크
			}
			for (j = 0; j < 4; j++) {
				game.getPlayer(0).getBall(i).hitBy(game.getPlayer(1).getBall(j)); // P1과 P2의 공끼리 체크
			}
		}

		//for (i = 0; i < 4; i++) {
		//	game.getPlayer(0).getBall(i).setScore();
		//	game.getPlayer(1).getBall(i).setScore();
		//} // 아래의 Player.setScore가 이 작업까지 수행

		// 점수 세팅
		//game.getPlayer(0).setScore();
		//game.getPlayer(1).setScore();
		game.setScoreC();

		// draw text
		g_player1.setStr(" " + std::to_string(game.getPlayer(0).getScore()));
		g_player2.setStr(" " + std::to_string(game.getPlayer(1).getScore()));
		g_totalP1.setStr("Player1: " + std::to_string(game.getScore(0)));
		g_totalP2.setStr("Player2: " + std::to_string(game.getScore(1)));
		g_set.setStr("SET " + std::to_string(game.getNowSet()) + "/" + std::to_string(game.getMaxSet()));
		g_score.draw(); 
		g_player1.draw();
		g_player2.draw();
		g_totalP1.draw();
		g_totalP2.draw();
		g_set.draw();
		winner.draw();

		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i=0;i<4;i++) 	{
			g_legowall[i].draw(Device, g_mWorld);
			game.getPlayer(0).getBall(i).draw(Device, g_mWorld);
			game.getPlayer(1).getBall(i).draw(Device, g_mWorld);
		}
		g_target_blueball.draw(Device, g_mWorld);
        g_light.draw(Device);
		
		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture( 0, NULL );
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;
	
	switch( msg ) {
	case WM_DESTROY:
        {
			::PostQuitMessage(0);
			break;
        }
	case WM_KEYDOWN:
        {
            switch (wParam) {
            case VK_ESCAPE:
				::DestroyWindow(hwnd);
                break;
            case VK_RETURN:
                if (NULL != Device) {
                    wire = !wire;
                    Device->SetRenderState(D3DRS_FILLMODE,
                        (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
                }
                break;
            case VK_SPACE:
				
				int i;
				int turn = game.getNowTurn();
				int player = game.getWhoseTurn();
				// 공이 멈출 때까지 기다림
				if (turn > 1) {
					for (i = 0; i < 4; i++) {
						if (!game.isAllStop()) goto HERE;
					}
				}

				D3DXVECTOR3 targetpos = g_target_blueball.getCenter();
				D3DXVECTOR3	whitepos = game.getPlayer(player).getBall(turn - 1).getCenter();

				double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
					pow(targetpos.z - whitepos.z, 2)));		// 기본 1 사분면
				if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }	//4 사분면
				if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 사분면
				if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 사분면
				double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
				game.getPlayer(player).getBall(turn - 1).setPower(distance * cos(theta), distance * sin(theta));
				game.getPlayer(player).getBall(turn - 1).setIsPlaying(true); // player가 공을 굴렸으면 게임에 참여중인 공이 된다.

				game.nextTurn();

			HERE:
				break;

			}
			break;
        }
		
	case WM_MOUSEMOVE:
        {
            int new_x = LOWORD(lParam);
            int new_y = HIWORD(lParam);
			float dx;
			float dy;
			
            if (LOWORD(wParam) & MK_LBUTTON) {
				
                if (isReset) {
                    isReset = false;
                } else {
                    D3DXVECTOR3 vDist;
                    D3DXVECTOR3 vTrans;
                    D3DXMATRIX mTrans;
                    D3DXMATRIX mX;
                    D3DXMATRIX mY;
					
                    switch (move) {
                    case WORLD_MOVE:
                        dx = (old_x - new_x) * 0.01f;
                        dy = (old_y - new_y) * 0.01f;
                        D3DXMatrixRotationY(&mX, dx);
                        D3DXMatrixRotationX(&mY, dy);
                        g_mWorld = g_mWorld * mX * mY;
						
                        break;
                    }
                }
				
                old_x = new_x;
                old_y = new_y;

            } else {
                isReset = true;
				
				if (LOWORD(wParam) & MK_RBUTTON) {
					dx = (old_x - new_x);// * 0.01f;
					dy = (old_y - new_y);// * 0.01f;
		
					D3DXVECTOR3 coord3d=g_target_blueball.getCenter();
					g_target_blueball.setCenter(coord3d.x+dx*(-0.007f),coord3d.y,coord3d.z+dy*0.007f );
				}
				old_x = new_x;
				old_y = new_y;
				
                move = WORLD_MOVE;
            }
            break;
        }
	}
	
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));
	
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}
	
	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}
	
	d3d::EnterMsgLoop( Display );
	
	Cleanup();
	
	Device->Release();
	
	return 0;
}