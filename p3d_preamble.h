/****************************************************************************
 * p3d_preamble.h
 * Author Joel Welling
 * Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/
/*
This include file provides the preamble that appears at the top of 
generated P3D files.
*/

/* Preamble is broken into segments to deal with compilers which do no
 * support long strings.
 */
static char preamble1[]= "\
;P3D 2.1 \n\
(load \"p3d.lsp\") \n\
(defun pt (xval yval zval) (make-point :x xval :y yval :z zval)) \n\
(defun vec (xval yval zval) (make-vector :x xval :y yval :z zval)) \n\
(defun clr (rval gval bval aval) \n\
  (make-color :r rval :g gval :b bval :a aval)) \n\
(defun trns (vals) (make-array '(4 4) :initial-contents vals)) \n\
(defun vx (xval yval zval) (make-vertex :x xval :y yval :z zval)) \n\
(defun vxc (xval yval zval clis) \n\
  (make-vertex :x xval :y yval :z zval :clr (apply 'clr clis))) \n\
"; /* end of preamble1 */
static char preamble2[]= "\
(defun vxcn (xval yval zval clis vlis) \n\
  (make-vertex :x xval :y yval :z zval \n\
               :clr (apply 'clr clis) :normal (apply 'vec vlis))) \n\
(defun vxn (xval yval zval clis vlis) \n\
  (make-vertex :x xval :y yval :z zval :normal (apply 'vec vlis))) \n\
(defun vl (vlist) (do ((vls vlist (cdr vls))) ((null vls) vlist) \n\
                       (rplaca vls (apply 'vx (car vls))))) \n\
(defun vlc (vlist) (do ((vls vlist (cdr vls))) ((null vls) vlist) \n\
                       (rplaca vls (apply 'vxc (car vls))))) \n\
(defun vlcn (vlist) (do ((vls vlist (cdr vls))) ((null vls) vlist) \n\
                       (rplaca vls (apply 'vxcn (car vls))))) \n\
"; /* end of preamble2 */
static char preamble3[]= "\
(defun vln (vlist) (do ((vls vlist (cdr vls))) ((null vls) vlist) \n\
                       (rplaca vls (apply 'vxn (car vls))))) \n\
(defun pm (vlist) (apply 'polymarker (vl vlist))) \n\
(defun pl (vlist) (apply 'polyline (vl vlist))) \n\
(defun pg (vlist) (apply 'polygon (vl vlist))) \n\
(defun bp (vlist) (apply 'bezier (vl vlist))) \n\
(defun tri (vlist) (apply 'triangle (vl vlist))) \n\
(defun pmc (vlist) (apply 'polymarker (vlc vlist))) \n\
(defun plc (vlist) (apply 'polyline (vlc vlist))) \n\
(defun pgc (vlist) (apply 'polygon (vlc vlist))) \n\
"; /* end of preamble3 */
static char preamble4[]= "\
(defun bpc (vlist) (apply 'bezier (vlc vlist))) \n\
(defun tric (vlist) (apply 'triangle (vlc vlist))) \n\
(defun pmcn (vlist) (apply 'polymarker (vlcn vlist))) \n\
(defun plcn (vlist) (apply 'polyline (vlcn vlist))) \n\
(defun pgcn (vlist) (apply 'polygon (vlcn vlist))) \n\
(defun bpcn (vlist) (apply 'bezier (vlcn vlist))) \n\
(defun tricn (vlist) (apply 'triangle (vlcn vlist))) \n\
(defun pmn (vlist) (apply 'polymarker (vln vlist))) \n\
(defun pln (vlist) (apply 'polyline (vln vlist))) \n\
(defun pgn (vlist) (apply 'polygon (vln vlist))) \n\
"; /* end of preamble4 */
static char preamble5[]= "\
(defun bpn (vlist) (apply 'bezier (vln vlist))) \n\
(defun trin (vlist) (apply 'triangle (vln vlist))) \n\
(defun msh (vcount vlist ilist) \n\
  (let ((varray (do ((result (make-array vcount) result) (i 0 (+ i 1)) \n\
                     (vls (vl vlist) (cdr vls))) ((null vls) result) \n\
                    (setf (aref result i) (car vls))))) \n\
    (mesh vlist (do ((ils ilist (cdr ils))) ((null ils) ilist) \n\
                    (do ((vls (car ils) (cdr vls))) ((null vls) T) \n\
                        (rplaca vls (aref varray (car vls)))))))) \n\
"; /* end of preamble5 */
static char preamble6[]= "\
(defun mshc (vcount vlist ilist) \n\
  (let ((varray (do ((result (make-array vcount) result) (i 0 (+ i 1)) \n\
                     (vls (vlc vlist) (cdr vls))) ((null vls) result) \n\
                    (setf (aref result i) (car vls))))) \n\
    (mesh vlist (do ((ils ilist (cdr ils))) ((null ils) ilist) \n\
                    (do ((vls (car ils) (cdr vls))) ((null vls) T) \n\
                        (rplaca vls (aref varray (car vls)))))))) \n\
(defun mshcn (vcount vlist ilist) \n\
  (let ((varray (do ((result (make-array vcount) result) (i 0 (+ i 1)) \n\
                     (vls (vlcn vlist) (cdr vls))) ((null vls) result) \n\
                    (setf (aref result i) (car vls))))) \n\
    (mesh vlist (do ((ils ilist (cdr ils))) ((null ils) ilist) \n\
                    (do ((vls (car ils) (cdr vls))) ((null vls) T) \n\
                        (rplaca vls (aref varray (car vls)))))))) \n\
"; /* end of preamble6 */
static char preamble7[]= "\
(defun mshn (vcount vlist ilist) \n\
  (let ((varray (do ((result (make-array vcount) result) (i 0 (+ i 1)) \n\
                     (vls (vln vlist) (cdr vls))) ((null vls) result) \n\
                    (setf (aref result i) (car vls))))) \n\
    (mesh vlist (do ((ils ilist (cdr ils))) ((null ils) ilist) \n\
                    (do ((vls (car ils) (cdr vls))) ((null vls) T) \n\
                        (rplaca vls (aref varray (car vls)))))))) \n\
(defun tx (str loc uvec vvec) \n\
  (text (apply 'pt loc) (apply 'vec uvec) (apply 'vec vvec) str)) \n\
(defun lt (loc color) (light (apply 'pt loc) (apply 'clr \n\
                                                    (append color '(1))))) \n\
(defun amb (color) (ambient (apply 'clr (append color '(1))))) \n\
"; /* end of preamble7 */
static char preamble8[]= "\
(setq default-lights (def-gob \n\
                :children (list \n\
                (light (make-point :x 0.0 :y 1.0 :z 20.0) \n\
                      (make-color :r 0.8 :g 0.8 :b 0.8)) \n\
                (ambient (make-color :r 0.3 :g 0.3 :b 0.3)) ))) \n\
(setq default-camera (make-camera \n\
        :lookfrom (make-point :x 0.0 :y 0.0 :z 20.0) \n\
        :lookat origin \n\
        :fovea 45.0 \n\
        :up y-vec \n\
        :hither -1.0 \n\
        :yon -50.0 )) \n\
;END PREAMBLE \n\
"; /* end of preamble8 */
