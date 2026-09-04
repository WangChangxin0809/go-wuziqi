#include <bits/stdc++.h>
using namespace std;
static const int N=15; using B=array<array<int,N>,N>;
struct M{int r,c;};
bool inside(int r,int c){return r>=0&&r<N&&c>=0&&c<N;}
int line(const B&b,int r,int c,int dr,int dc,int p){int n=1; for(int s:{-1,1}){int x=r+dr*s,y=c+dc*s;while(inside(x,y)&&b[x][y]==p)n++,x+=dr*s,y+=dc*s;}return n;}
bool win(const B&b,int r,int c,int p){for(auto d:array<pair<int,int>,4>{{{1,0},{0,1},{1,1},{1,-1}}})if(line(b,r,c,d.first,d.second,p)>=5)return true;return false;}
bool forbidden(const B&b,int r,int c){ // conservative Renju black foul: overline or double open-four/open-three
 if(b[r][c]>=0)return true; B q=b;q[r][c]=0; if(line(q,r,c,1,0,0)>5||line(q,r,c,0,1,0)>5||line(q,r,c,1,1,0)>5||line(q,r,c,1,-1,0)>5)return true;
 int fours=0,threes=0; for(auto d:array<pair<int,int>,4>{{{1,0},{0,1},{1,1},{1,-1}}}){int n=line(q,r,c,d.first,d.second,0); if(n>=4)fours++; if(n==3){int a=0;for(int s:{-1,1}){int x=r+d.first*s,y=c+d.second*s; if(inside(x,y)&&q[x][y]<0)a++;}if(a==2)threes++;}} return fours>=2||threes>=2;
}
vector<M> moves(const B&b,int me){vector<M>v; bool any=false;for(int r=0;r<N;r++)for(int c=0;c<N;c++)if(b[r][c]>=0)any=true; if(!any)return {{7,7}}; for(int r=0;r<N;r++)for(int c=0;c<N;c++)if(b[r][c]<0&&!(me==0&&forbidden(b,r,c))){bool near=false;for(int i=max(0,r-2);i<=min(N-1,r+2);i++)for(int j=max(0,c-2);j<=min(N-1,c+2);j++)near|=b[i][j]>=0;if(near)v.push_back({r,c});} return v;}
int threat(const B&b,M m,int p){B q=b;q[m.r][m.c]=p;int s=0;for(auto d:array<pair<int,int>,4>{{{1,0},{0,1},{1,1},{1,-1}}}){int n=line(q,m.r,m.c,d.first,d.second,p);s+= n>=5?100000:n==4?4000:n==3?250:n==2?20:1;}return s;}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int me;if(!(cin>>me))return 0;B b{};int x;for(auto&r:b)for(int&v:r)cin>>v;int opp=1-me;auto cand=moves(b,me); if(cand.empty()){cout<<7<<' '<<7;return 0;}
 // deterministic tactical pre-pass: win, then prevent opponent win.
 for(M m:cand){B q=b;q[m.r][m.c]=me;if(win(q,m.r,m.c,me)){cout<<m.r<<' '<<m.c;return 0;}}
 for(M m:cand){B q=b;q[m.r][m.c]=opp;if(win(q,m.r,m.c,opp)){cout<<m.r<<' '<<m.c;return 0;}}
 mt19937 rng(0x9e3779b9u+(unsigned)cand.size()*17); vector<int> score(cand.size());
 // Root-sampled Monte Carlo: tactical weighted rollouts, intentionally unlike alpha-beta.
 for(size_t k=0;k<cand.size();k++){for(int t=0;t<28;t++){B q=b;q[cand[k].r][cand[k].c]=me;int turn=opp;int val=threat(q,cand[k],me)-threat(q,cand[k],opp)/2;for(int d=0;d<3;d++){auto v=moves(q,turn);if(v.empty())break;sort(v.begin(),v.end(),[&](M a,M z){return threat(q,a,turn)>threat(q,z,turn);});int lim=min<int>(v.size(), min(6,2+(int)(rng()%5)));M m=v[rng()%lim];q[m.r][m.c]=turn;if(win(q,m.r,m.c,turn)){val+=(turn==me?12000:-14000);break;}turn=1-turn;}score[k]+=val;}}
 size_t best=0;for(size_t i=1;i<cand.size();i++)if(score[i]>score[best])best=i;cout<<cand[best].r<<' '<<cand[best].c<<'\n';}
