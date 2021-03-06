#ifndef __gui_projection_h__
#define __gui_projection_h__

#include "gui/opengl/gl.h"
#include "gui/opengl/font.h"
#include "gui/opengl/transformation.h"
#include "point.h"
#include "math/matrix.h"
#include "math/LU.h"

namespace MR
{
  namespace GUI
  {

    const int TopEdge = 0x00000001;
    const int BottomEdge = 0x00000002;
    const int LeftEdge = 0x00000004;
    const int RightEdge = 0x00000008;


    class Projection
    {
      public:
        Projection (QGLWidget* parent, const GL::Font& font) : 
          glarea (parent), 
          font (font) { } 

        void set_viewport (const QWidget& frame, int x, int y, int w, int h) {
          viewport[0] = x;
          viewport[1] = y;
          viewport[2] = w;
          viewport[3] = h;
          set_viewport (frame);
        }

        void set_viewport (const QWidget& frame) const {
#if QT_VERSION >= 0x050100
          int m = frame.windowHandle()->devicePixelRatio();
          gl::Viewport (m*viewport[0], m*viewport[1], m*viewport[2], m*viewport[3]);
#else
          gl::Viewport (viewport[0], viewport[1], viewport[2], viewport[3]);
#endif
        }



        void set (const GL::mat4& modelview, const GL::mat4& projection) {
          MV = modelview;
          P = projection;
          MVP = P * MV;

          iMV = GL::inv (MV);
          iP = GL::inv (P);
          iMVP = iMV * iP;
        }

        GLint x_position () const {
          return viewport[0];
        }

        GLint y_position () const {
          return viewport[1];
        }

        GLint width () const {
          return viewport[2];
        }

        GLint height () const {
          return viewport[3];
        }

        float depth_of (const Point<>& x) const {
          float d = MVP(2,0)*x[0] + MVP(2,1)*x[1] + MVP(2,2)*x[2] + MVP(2,3);
          if (MVP(3,2)) d /= MVP(3,0)*x[0] + MVP(3,1)*x[1] + MVP(3,2)*x[2] + MVP(3,3);
          return d;
        }

        Point<> model_to_screen (const Point<>& x) const {
          Point<> S (
              MVP(0,0)*x[0] + MVP(0,1)*x[1] + MVP(0,2)*x[2] + MVP(0,3),
              MVP(1,0)*x[0] + MVP(1,1)*x[1] + MVP(1,2)*x[2] + MVP(1,3),
              MVP(2,0)*x[0] + MVP(2,1)*x[1] + MVP(2,2)*x[2] + MVP(2,3));
          if (MVP(3,2))
            S /= MVP(3,0)*x[0] + MVP(3,1)*x[1] + MVP(3,2)*x[2] + MVP(3,3);
          S[0] = viewport[0] + 0.5f*viewport[2]*(1.0f+S[0]); 
          S[1] = viewport[1] + 0.5f*viewport[3]*(1.0f+S[1]); 
          return S;
        }

        Point<> model_to_screen_direction (const Point<>& dir) const {
          Point<> S (
              MVP(0,0)*dir[0] + MVP(0,1)*dir[1] + MVP(0,2)*dir[2],
              MVP(1,0)*dir[0] + MVP(1,1)*dir[1] + MVP(1,2)*dir[2],
              MVP(2,0)*dir[0] + MVP(2,1)*dir[1] + MVP(2,2)*dir[2]);
          S[0] *= 0.5f*viewport[2];
          S[1] *= 0.5f*viewport[3];
          return S;
        }

        Point<> screen_to_model (float x, float y, float depth) const {
          x = 2.0f*(x-viewport[0])/viewport[2] - 1.0f;
          y = 2.0f*(y-viewport[1])/viewport[3] - 1.0f;
          Point<> S (
              iMVP(0,0)*x + iMVP(0,1)*y + iMVP(0,2)*depth + iMVP(0,3),
              iMVP(1,0)*x + iMVP(1,1)*y + iMVP(1,2)*depth + iMVP(1,3),
              iMVP(2,0)*x + iMVP(2,1)*y + iMVP(2,2)*depth + iMVP(2,3));
          if (MVP(3,2)) 
            S /= iMVP(3,0)*x + iMVP(3,1)*y + iMVP(3,2)*depth + iMVP(3,3);
          return S;
        }

        Point<> screen_to_model (const Point<>& x) const {
          return screen_to_model (x[0], x[1], x[2]);
        }

        Point<> screen_to_model (const Point<>& x, float depth) const {
          return screen_to_model (x[0], x[1], depth);
        }

        Point<> screen_to_model (const Point<>& x, const Point<>& depth) const {
          return screen_to_model (x, depth_of (depth));
        }

        Point<> screen_to_model (const QPoint& x, float depth) const {
          return screen_to_model (x.x(), x.y(), depth);
        }

        Point<> screen_to_model (const QPoint& x, const Point<>& depth) const {
          return screen_to_model (x, depth_of (depth));
        }

        Point<> screen_normal () const {
          return Point<> (iMVP(0,2), iMVP(1,2), iMVP(2,2)).normalise();
        }

        Point<> screen_to_model_direction (float x, float y, float depth) const {
          x *= 2.0f/viewport[2];
          y *= 2.0f/viewport[3];
          Point<> S (iMVP(0,0)*x + iMVP(0,1)*y, iMVP(1,0)*x + iMVP(1,1)*y, iMVP(2,0)*x + iMVP(2,1)*y);
          if (MVP(3,2)) 
            S /= iMVP(3,2)*depth + iMVP(3,3);
          return S;
        }

        Point<> screen_to_model_direction (const Point<>& dx, float x) const {
          return screen_to_model_direction (dx[0], dx[1], x);
        }

        Point<> screen_to_model_direction (const Point<>& dx, const Point<>& x) const {
          return screen_to_model_direction (dx, depth_of (x));
        }

        Point<> screen_to_model_direction (const QPoint& dx, float x) const {
          return screen_to_model_direction (dx.x(), dx.y(), x);
        }

        Point<> screen_to_model_direction (const QPoint& dx, const Point<>& x) const {
          return screen_to_model_direction (dx, depth_of (x));
        }


        void render_crosshairs (const Point<>& focus) const;

        void setup_render_text (float red = 1.0, float green = 1.0, float blue = 0.0) const { 
          font.start (width(), height(), red, green, blue); 
        }
        void done_render_text () const { font.stop(); }

        void render_text (int x, int y, const std::string& text) const {
          font.render (text, x, y);
        }

        void render_text_align (int x, int y, const std::string& text, int halign = 0, int valign = 0) const {
          QString s (text.c_str());
          int w = font.metric.width (s);
          int h = font.metric.height();
          if (halign == 0) x -= w/2;
          else if (halign > 0) x -= w;
          if (valign == 0) y -= h/2;
          else if (valign > 0) y -= h;
          render_text (x, y, text);
        }

        void render_text_inset (int x, int y, const std::string& text, int inset = -1) const {
          QString s (text.c_str());
          if (inset < 0) 
            inset = font.metric.height() / 2;
          if (x < inset) 
            x = inset;
          if (x + font.metric.width (s) + inset > width()) 
            x = width() - font.metric.width (s) - inset;
          if (y < inset) 
            y = inset;
          if (y + font.metric.height() + inset > height())
            y = height() - font.metric.height() - inset;
          render_text (x, y, text);
        }

        void render_text (const std::string& text, int position, int line = 0) const {
          QString s (text.c_str());
          int x, y;

          if (position & RightEdge) x = width() - font.metric.height() / 2 - font.metric.width (s);
          else if (position & LeftEdge) x = font.metric.height() / 2;
          else x = (width() - font.metric.width (s)) / 2;

          if (position & TopEdge) y = height() - 1.5 * font.metric.height() - line * font.metric.lineSpacing();
          else if (position & BottomEdge) y = font.metric.height() / 2 + line * font.metric.lineSpacing();
          else y = (height() - font.metric.height()) / 2 - line * font.metric.lineSpacing();

          render_text (x, y, text);
        }

        void draw_orientation_labels () const;


        const GL::mat4& modelview_projection () const { return MVP; }
        const GL::mat4& modelview_projection_inverse () const { return iMVP; }
        const GL::mat4& modelview () const { return MV; }
        const GL::mat4& modelview_inverse () const { return iMV; }
        const GL::mat4& projection () const { return P; }
        const GL::mat4& projection_inverse () const { return iP; }


        void set (GL::Shader::Program& shader_program) const {
          assert (shader_program != 0);
          gl::UniformMatrix4fv (gl::GetUniformLocation (shader_program, "MVP"), 1, gl::FALSE_, modelview_projection());
        }

      protected:
        QGLWidget* glarea;
        const GL::Font& font;
        GL::mat4 MV, iMV, P, iP, MVP, iMVP;
        GLint viewport[4];
        mutable GL::VertexBuffer crosshairs_VB;
        mutable GL::VertexArrayObject crosshairs_VAO;
        mutable GL::Shader::Program crosshairs_program;
    };




  }
}

#endif
