package com.temple.ffmpegplayer

import android.content.Context
import android.util.AttributeSet
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView

class HYSurfaceView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null
) : SurfaceView(context, attrs), SurfaceHolder.Callback, Choreographer.FrameCallback {

    private var player: HYPlayer? = null
    private var released = false

    init {
        holder.addCallback(this)
        Choreographer.getInstance().postFrameCallback(this)
    }

    fun setPlayer(player: HYPlayer) {
        this.player = player
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        player?.createSurface(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        player?.changeSurface(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        player?.destroySurface(holder.surface)
    }

    override fun doFrame(frameTimeNanos: Long) {
        if (released) {
            return
        }
        player?.doFrame()
        Choreographer.getInstance().postFrameCallback(this)
    }

    fun release() {
        released = true
        Choreographer.getInstance().postFrameCallback(this)
    }

}