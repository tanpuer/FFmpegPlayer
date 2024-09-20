package com.temple.ffmpegplayer

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import androidx.appcompat.widget.AppCompatSeekBar
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat

class MainActivity : AppCompatActivity(), HYPlayerCallback, OnSeekBarChangeListener {

    private lateinit var ivPlay: ImageView
    private lateinit var ivPrevious: ImageView
    private lateinit var ivNext: ImageView
    private lateinit var seekBar: AppCompatSeekBar
    private lateinit var player: HYPlayer
    private lateinit var surfaceView: HYSurfaceView
    private lateinit var currTextView: TextView
    private lateinit var totalTextView: TextView
    private var duration = 1L
    private var isSeeking = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val controller = WindowCompat.getInsetsController(window, window.decorView)
        controller.hide(WindowInsetsCompat.Type.statusBars())
        setContentView(R.layout.activity_main)
        ivPlay = findViewById(R.id.ivPlay)
        ivPrevious = findViewById(R.id.ivPrevious)
        ivNext = findViewById(R.id.ivNext)
        seekBar = findViewById(R.id.seekBar)
        surfaceView = findViewById(R.id.surface_view)
        currTextView = findViewById(R.id.tvCurrTime)
        totalTextView = findViewById(R.id.tvTotalTime)

        ivPlay.setOnClickListener {
            if (player.isPlaying()) {
                player.pause()
                ivPlay.setImageResource(R.drawable.ic_play)
            } else {
                player.start()
                ivPlay.setImageResource(R.drawable.ic_pause)
            }
        }
        ivPrevious.setOnClickListener {
            player.seek(0)
        }
        ivNext.setOnClickListener {
            player.seek(0)
        }

        player = HYPlayer()
        surfaceView.setPlayer(player)
        player.setSource("feng.mp4")
        player.start()
        ivPlay.setImageResource(R.drawable.ic_pause)
        player.setCallback(this)
        seekBar.setOnSeekBarChangeListener(this)
    }

    override fun onPrepared(
        audioDuration: Long,
        videoDuration: Long,
        videoWidth: Int,
        videoHeight: Int
    ) {
        duration = Math.max(audioDuration, videoDuration)
        totalTextView.text = Utils.formatTime(duration)
    }

    override fun onProgressUpdate(currPos: Long) {
        currTextView.text = Utils.formatTime(currPos)
        if (isSeeking) {
            return
        }
        seekBar.progress = (currPos * 100.0f / duration).toInt()
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
        isSeeking = true
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        player.seek(this.seekBar.progress * duration / 100)
        isSeeking = false
    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {

    }

}