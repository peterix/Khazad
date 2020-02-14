/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naessén and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naessén a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GCN_TOGGLEIMAGEBUTTON_HPP
#define GCN_TOGGLEIMAGEBUTTON_HPP

#include "guichan/platform.hpp"
#include "guichan/widgets/button.hpp"

namespace gcn
{
    class Image;

    /**
     * An implementation of a regular clickable button. Unlike a normal button an image 
     * button is capable of displaying an image instead of a simple text caption. 
     * Whenever an image button is clicked an action event will be sent to the action 
     * listener's of the image button.
     *
     * @see Button
     */
    class GCN_CORE_DECLSPEC KhazToggleButton : public gcn::Button
    {
    public:
        /**
         * Default constructor.
         */
        KhazToggleButton();

        /**
         * Constructor.
         *
         * @param onfilename The filename of the image to display in on state.
         * @param offfilename The filename of the image to display in off state.
         */
        KhazToggleButton(const std::string& onfilename,const std::string& offfilename);

        /**
         * Constructor.
         *
         * @param onimage The image to display in on state.
         * @param offimage The image to display in off state.
         */
        KhazToggleButton(const Image* onimage, const Image* offimage);

        /**
         * Destructor.
         */
        virtual ~KhazToggleButton();

        /**
         * Sets the On image to display. Existing Image is freed automatically, 
         * if it was loaded internally.
         *
         * @param image The image to display.
         */
        void setOnImage(const Image* image);
        
        /**
         * Sets the Off image to display. Existing Image is freed automatically, 
         * if it was loaded internally.
         *
         * @param image The image to display.
         */
        void setOffImage(const Image* image);

        /**
         * Gets current On image.
         *
         * @return The current On image.
         */
        const Image* getOnImage() const;

        /**
         * Gets current Off image.
         *
         * @return The current Off image.
         */
        const Image* getOffImage() const;

        // Inherited from Widget

        void draw(gcn::Graphics* graphics);
        
        // Inherited from Button
        virtual void mouseReleased(MouseEvent& mouseEvent);
    protected:
        /**
         * The image to display in On state.
         */
        const Image* mImageOn;
        
        /**
         * The image to display in Off state.
         */
        const Image* mImageOff;
        
        /**
         * True if the image has been loaded internally, false otherwise.
         * An image not loaded internally should not be deleted in the
         * destructor.
         */
        bool mInternalImageOn;
        
        /**
         * True if the image has been loaded internally, false otherwise.
         * An image not loaded internally should not be deleted in the
         * destructor.
         */
        bool mInternalImageOff;
        
        /**
        * On or Off
        */
        bool State;
    };
}
#endif
