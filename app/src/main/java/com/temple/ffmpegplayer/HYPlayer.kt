package com.temple.ffmpegplayer

import android.content.res.AssetManager
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.util.Log
import android.view.Surface

class HYPlayer {

    private val index = INDEX++

    private var player = 0L
    private var audioPlayer = 0L
    private var videoPlayer = 0L

    @Volatile
    private var isEnd = false

    @Volatile
    private var videoBufferSize = 0

    @Volatile
    private var isDemuxing = true

    private var isPlaying = false

    @Volatile
    private var currAudioPts = 0L

    @Volatile
    private var currVideoPts = 0L

    @Volatile
    private var videoDuration = 0L

    @Volatile
    private var audioDuration = 0L
    private var lastTime = 0L

    @Volatile
    private var demuxComplete = false

    @Volatile
    private var loop = false

    private val demuxerThread: HandlerThread =
        HandlerThread("demuxer-${index}", Thread.MAX_PRIORITY).apply {
            start()
        }
    private val demuxer = Handler(demuxerThread.looper)

    private val videoDecoderThread: HandlerThread =
        HandlerThread("video-decoder-${index}", Thread.MAX_PRIORITY).apply {
            start()
        }
    private val videoDecoder = Handler(videoDecoderThread.looper)

    private val audioDecoderThread: HandlerThread =
        HandlerThread("audio-decoder-${index}", Thread.MAX_PRIORITY).apply {
            start()
        }
    private val audioDecoder = Handler(audioDecoderThread.looper)

    private val audioPlayerThread: HandlerThread =
        HandlerThread("audio-player-${index}", Thread.MAX_PRIORITY).apply {
            start()
        }
    private val audioHandler = Handler(audioPlayerThread.looper)

    private val glThread: HandlerThread =
        HandlerThread("gl-${index}", Thread.MAX_PRIORITY).apply {
            start()
        }
    private val glHandler = Handler(glThread.looper)

    private val mainHandler = Handler(Looper.getMainLooper())
    private var playerCallback: HYPlayerCallback? = null

    init {
        demuxer.post {
            player = nativeInit(HYPlayerApp.getInstance().assets)
        }
        audioHandler.post {
            audioPlayer = nativeCreateAudioPlayer()
        }
        glHandler.post {
            videoPlayer = nativeCreateVideoPlayer(HYPlayerApp.getInstance().assets)
        }
    }

//API start-----------------------------------------------------------------------------------------

    fun setSource(source: String) {
        demuxer.post {
            nativeSetSource(player, source)
        }
        demuxer.removeCallbacks(demuxRunnable)
        demuxComplete = false
        isEnd = false
        isDemuxing = true
        demuxer.post(demuxRunnable)
        glHandler.post {
            nativeSetTitle(videoPlayer, source)
        }
    }

    fun start() {
        isPlaying = true
        audioHandler.post {
            nativeStartAudio(audioPlayer)
        }
        doFrame()
    }

    fun pause() {
        isPlaying = false
        audioHandler.post {
            nativePauseAudio(audioPlayer)
        }
    }

    fun isPlaying(): Boolean {
        return isPlaying
    }

    fun seek(timeMills: Long) {
        isDemuxing = false
        pause()

        demuxer.post {
            nativeSeek(player, timeMills)
            demuxComplete = false
            isEnd = false

            audioDecoder.post {
                nativeFlushAudio(player)
                audioHandler.post {
                    nativeClearAudioBuffer(audioPlayer)
                }
            }
            videoDecoder.post {
                nativeFlushVideo(player)
                glHandler.post {
                    nativeClearVideoBuffer(videoPlayer)
                }
            }

            updateVideoBufferInfo(0)
            mainHandler.post {
                start()
                playerCallback?.onSeek(timeMills)
            }
        }
    }

    fun setCallback(callback: HYPlayerCallback) {
        playerCallback = callback
    }

    fun getCurrentPts(): Long {
        return currAudioPts
    }

    fun setLoop(loop: Boolean) {
        this.loop = loop
    }

//API end-------------------------------------------------------------------------------------------

    private val demuxRunnable = object : Runnable {

        override fun run() {
            if (isEnd) {
                return
            }
            Log.d(TAG, "readOnePacket")
            val result = nativeReadOnePacket(player)
            when (result) {
                READ_PACKET_NULL -> {

                }

                READ_PACKET_END -> {
                    isEnd = true
                }

                READ_PACKET_AUDIO -> {

                }

                READ_PACKET_VIDEO -> {

                }

                else -> {
                }
            }
            if (isDemuxing) {
                demuxer.postDelayed(this, 5)
            }
        }

    }

    //gl start--------------------------------------------------------------------------------------
    fun createSurface(surface: Surface) {
        glHandler.post {
            nativeCreateSurface(videoPlayer, surface)
        }
    }

    fun changeSurface(width: Int, height: Int) {
        glHandler.post {
            nativeChangeSurface(videoPlayer, width, height)
        }
        doFrame(true)
    }

    fun destroySurface(surface: Surface) {
        glHandler.post {
            nativeDestroySurface(videoPlayer, surface)
        }
    }

    fun doFrame(force: Boolean = false) {
        if (videoBufferSize == 0 || (!isPlaying && !force)) {
            return
        }
        audioHandler.post {
            currAudioPts = nativeGetAudioPts(audioPlayer)
        }
        glHandler.post {
            currVideoPts = nativeGetVideoPts(videoPlayer)
            if ((audioDuration - currAudioPts) < 32) {
                //视频向音频同步，处理处理音频时长 < 视频时长的特殊情况
                if (lastTime == 0L) {
                    lastTime = System.currentTimeMillis()
                }
                nativeDoFrame(videoPlayer, currAudioPts + System.currentTimeMillis() - lastTime)
            } else {
                lastTime = 0L
                nativeDoFrame(videoPlayer, currAudioPts)
            }
        }
        mainHandler.post {
            playerCallback?.onProgressUpdate(Math.max(currAudioPts, currVideoPts))
        }
    }
    //gl end----------------------------------------------------------------------------------------

    // called from C++ start------------------------------------------------------------------------
    private fun onPrepared(
        audioDuration: Long,
        videoDuration: Long,
        videoWidth: Int,
        videoHeight: Int
    ) {
        this.videoDuration = videoDuration
        this.audioDuration = audioDuration
        mainHandler.post {
            playerCallback?.onPrepared(audioDuration, videoDuration, videoWidth, videoHeight)
        }
    }

    private fun sendAudioPacket(packet: Long) {
        audioDecoder.post {
            nativeSendAudioPacket(player, packet)
        }
    }

    private fun sendVideoPacket(packet: Long) {
        videoDecoder.post {
            nativeSendVideoPacket(player, packet)
        }
    }

    private fun sendAudioFrame(data: Long) {
        audioHandler.post {
            nativeSendAudioData(audioPlayer, data)
        }
    }

    private fun sendVideoFrame(data: Long) {
        glHandler.post {
            videoBufferSize = nativeSendVideoData(videoPlayer, data)
            updateVideoBufferInfo(videoBufferSize)
        }
    }

    private fun updateVideoBufferInfo(videoBufferSize: Int) {
        if (videoBufferSize < VIDEO_BUFFER_MIN && !isDemuxing) {
            demuxer.removeCallbacks(demuxRunnable)
            isDemuxing = true
            demuxer.post(demuxRunnable)
        } else if (videoBufferSize > VIDEO_BUFFER_MAX && isDemuxing) {
            isDemuxing = false
            demuxer.removeCallbacks(demuxRunnable)
        }
        if (videoBufferSize == 0 && demuxComplete) {
            mainHandler.post {
                playerCallback?.onComplete()
            }
            if (loop) {
                seek(0)
            }
        }
    }

    private fun onDemuxComplete() {
        demuxComplete = true
    }
    // called from C++ end--------------------------------------------------------------------------

    private external fun nativeInit(assetManager: AssetManager): Long
    private external fun nativeSetSource(player: Long, source: String)
    private external fun nativeSendAudioPacket(player: Long, packet: Long)
    private external fun nativeSendVideoPacket(player: Long, packet: Long)
    private external fun nativeReadOnePacket(player: Long): Int // -1=null, -2=end, 1=audio, 2=video
    private external fun nativeSeek(player: Long, timeMills: Long)
    private external fun nativeFlushAudio(player: Long)
    private external fun nativeFlushVideo(player: Long)

    private external fun nativeCreateVideoPlayer(assetManager: AssetManager): Long
    private external fun nativeSendVideoData(videoPlayer: Long, data: Long): Int
    private external fun nativeCreateSurface(videoPlayer: Long, surface: Surface)
    private external fun nativeChangeSurface(videoPlayer: Long, width: Int, height: Int)
    private external fun nativeDestroySurface(videoPlayer: Long, surface: Surface)
    private external fun nativeDoFrame(videoPlayer: Long, time: Long)
    private external fun nativeGetVideoPts(videoPlayer: Long): Long
    private external fun nativeClearVideoBuffer(videoPlayer: Long)
    private external fun nativeSetTitle(videoPlayer: Long, title: String)

    private external fun nativeCreateAudioPlayer(): Long
    private external fun nativeSendAudioData(audioPlayer: Long, data: Long)
    private external fun nativeGetAudioPts(audioPlayer: Long): Long
    private external fun nativeStartAudio(audioPlayer: Long)
    private external fun nativePauseAudio(audioPlayer: Long)
    private external fun nativeClearAudioBuffer(audioPlayer: Long)

    companion object {
        init {
            System.loadLibrary("hyplayer")
        }

        private const val TAG = "HYOpenGLPlayer"
        var INDEX = 1
        private const val READ_PACKET_NULL = -1
        private const val READ_PACKET_END = -2
        private const val READ_PACKET_AUDIO = 1
        private const val READ_PACKET_VIDEO = 2

        private const val VIDEO_BUFFER_MAX = 20
        private const val VIDEO_BUFFER_MIN = 10
    }

}