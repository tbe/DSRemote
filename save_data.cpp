/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2015 - 2020 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/


#define SAV_MEM_BSZ    (250000)



void UI_Mainwindow::save_screenshot()
{
  int n;

  QString errStr, opath;

  QPainter painter;

  QPainterPath path;

  if(device == NULL)
  {
    return;
  }

  scrn_timer->stop();

  scrn_thread->wait();

  tmc_write(":DISP:DATA?");

  save_data_thread get_data_thrd(0);

  QMessageBox w_msg_box;
  w_msg_box.setIcon(QMessageBox::NoIcon);
  w_msg_box.setText("Downloading data...");
  w_msg_box.setStandardButtons(QMessageBox::Abort);

  connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));

  get_data_thrd.start();

  if(w_msg_box.exec() != QDialog::Accepted)
  {
    disconnect(&get_data_thrd, 0, 0, 0);
    errStr =  "Aborted by user.";
    goto OUT_ERROR;
  }

  disconnect(&get_data_thrd, 0, 0, 0);

  n = get_data_thrd.get_num_bytes_rcvd();
  if(n < 0)
  {
    errStr = "Can not read from device.";
    goto OUT_ERROR;
  }

  if(device->sz != SCRN_SHOT_BMP_SZ)
  {
    errStr = "Error, bitmap has wrong filesize";
    goto OUT_ERROR;
  }

  if(strncmp(device->buf, "BM", 2))
  {
    errStr = "Error, file is not a bitmap";
    goto OUT_ERROR;
  }

  memcpy(devparms.screenshot_buf, device->buf, SCRN_SHOT_BMP_SZ);

  screenXpm.loadFromData((uchar *)(devparms.screenshot_buf), SCRN_SHOT_BMP_SZ);

  if(devparms.modelserie == 1)
  {
    painter.begin(&screenXpm);
#if QT_VERSION >= 0x050000
    painter.setRenderHint(QPainter::Qt4CompatiblePainting, true);
#endif

    painter.fillRect(0, 0, 80, 29, Qt::black);

    painter.setPen(Qt::white);

    painter.drawText(5, 8, 65, 20, Qt::AlignCenter, devparms.modelname);

    painter.end();
  }
  else if(devparms.modelserie == 6)
    {
      painter.begin(&screenXpm);
#if QT_VERSION >= 0x050000
      painter.setRenderHint(QPainter::Qt4CompatiblePainting, true);
#endif

      painter.fillRect(0, 0, 95, 29, QColor(48, 48, 48));

      path.addRoundedRect(5, 5, 85, 20, 3, 3);

      painter.fillPath(path, Qt::black);

      painter.setPen(Qt::white);

      painter.drawText(5, 5, 85, 20, Qt::AlignCenter, devparms.modelname);

      painter.end();
    }

  if(devparms.screenshot_inv)
  {
    screenXpm.invertPixels(QImage::InvertRgb);
  }

  if(!recent_savedir.isEmpty())
  {
    opath = recent_savedir;
    opath += "/";
  }
  opath += "screenshot.png";

  opath = QFileDialog::getSaveFileName(this, "Save file", opath, "PNG files (*.png *.PNG)");

  if(opath.isEmpty())
  {
    scrn_timer->start(devparms.screentimerival);

    return;
  }
  recent_savedir = directory_from_path(opath);


  if(!screenXpm.save(opath, "PNG", 50))
  {
    errStr = "Could not save file (unknown error)";
    goto OUT_ERROR;
  }

  scrn_timer->start(devparms.screentimerival);

  return;

OUT_ERROR:

  if(!get_data_thrd.isFinished())
  {
    connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));
    w_msg_box.setText("Waiting for thread to finish, please wait...");
    w_msg_box.exec();
  }

  scrn_timer->start(devparms.screentimerival);

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Critical);
  msgBox.setText(errStr);
  msgBox.exec();
}


void UI_Mainwindow::get_deep_memory_waveform()
{
  int i, k,
      n=0,
      chn,
      chns=0,
      bytes_rcvd=0,
      mempnts,
      yref[MAX_CHNS],
      empty_buf;

  short *wavbuf[MAX_CHNS];

  QString errStr, cmdStr;

  QEventLoop ev_loop;

  QMessageBox wi_msg_box;

  save_data_thread get_data_thrd(0);

  if(device == NULL)
  {
    return;
  }

  scrn_timer->stop();

  scrn_thread->wait();

  for(i=0; i<MAX_CHNS; i++)
  {
    wavbuf[i] = NULL;
  }

  mempnts = devparms.acquirememdepth;

  QProgressDialog progress("Downloading data...", "Abort", 0, mempnts, this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);

  connect(&get_data_thrd, SIGNAL(finished()), &ev_loop, SLOT(quit()));

  statusLabel->setText("Downloading data...");

  for(i=0; i<MAX_CHNS; i++)
  {
    if(!devparms.chandisplay[i])
    {
      continue;
    }

    chns++;
  }

  if(!chns)
  {
    errStr = "No active channels.";
    goto OUT_ERROR;
  }

  if(mempnts < 1)
  {
    errStr = "Can not download waveform when memory depth is set to \"Auto\".";
    goto OUT_ERROR;
  }

  for(i=0; i<MAX_CHNS; i++)
  {
    if(!devparms.chandisplay[i])  // Download data only when channel is switched on
    {
      continue;
    }

    wavbuf[i] = (short *)malloc(mempnts * sizeof(short));
    if(wavbuf[i] == NULL)
    {
      errStr = QString("Malloc error.  line %1 file %1").arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }
  }

  tmc_write(":STOP");

  usleep(20000);

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])  // Download data only when channel is switched on
    {
      continue;
    }

    progress.setLabelText(QString("Downloading channel %1 waveform data...").arg(chn+1));

    tmc_write(QString(":WAV:SOUR CHAN%1").arg(chn +1).toLocal8Bit().constData());

    tmc_write(":WAV:FORM BYTE");

    usleep(20000);

    tmc_write(":WAV:MODE RAW");

    usleep(20000);

    tmc_write(":WAV:YINC?");

    usleep(20000);

    tmc_read();

    devparms.yinc[chn] = atof(device->buf);

    if(devparms.yinc[chn] < 1e-6)
    {
      errStr = QString("Error, parameter \"YINC\" out of range: %1  line %2 file %3").arg(devparms.yinc[chn]).arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }

    usleep(20000);

    tmc_write(":WAV:YREF?");

    usleep(20000);

    tmc_read();

    yref[chn] = atoi(device->buf);

    if((yref[chn] < 1) || (yref[chn] > 255))
    {
      errStr = QString("Error, parameter \"YREF\" out of range: %1  line %2 file %3")
          .arg(yref[chn]).arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }

    usleep(20000);

    tmc_write(":WAV:YOR?");

    usleep(20000);

    tmc_read();

    devparms.yor[chn] = atoi(device->buf);

    if((devparms.yor[chn] < -255) || (devparms.yor[chn] > 255))
    {
      errStr = QString("Error, parameter \"YOR\" out of range: %1  line %2 file %3")
          .arg(devparms.yor[chn]).arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }

    empty_buf = 0;

    for(bytes_rcvd=0; bytes_rcvd<mempnts ;)
    {
      progress.setValue(bytes_rcvd);

      if(progress.wasCanceled())
      {
        errStr = "Canceled";
        goto OUT_ERROR;
      }

      usleep(20000);

      tmc_write(QString(":WAV:STAR %1").arg(bytes_rcvd +1).toLocal8Bit().constData());

      if((bytes_rcvd + SAV_MEM_BSZ) > mempnts)
      {
        cmdStr = QString(":WAV:STOP %1").arg(mempnts);
      }
      else
      {
        cmdStr = QString(":WAV:STOP %1").arg(bytes_rcvd + SAV_MEM_BSZ);
      }

      usleep(20000);

      tmc_write(cmdStr.toLocal8Bit().constData());

      usleep(20000);

      tmc_write(":WAV:DATA?");

      get_data_thrd.start();

      ev_loop.exec();

      n = get_data_thrd.get_num_bytes_rcvd();
      if(n < 0)
      {
        errStr = QString("Can not read from device.  line %i file %s").arg(__LINE__).arg(__FILE__);
        goto OUT_ERROR;
      }

      if(n == 0)
      {
        errStr = "No waveform data available.";
        goto OUT_ERROR;
      }

      qDebug() << QString("received %1 bytes, total %1 bytes").arg(n).arg(n + bytes_rcvd);

      if(n > SAV_MEM_BSZ)
      {
        errStr = QString("Datablock too big for buffer: %1  line %2 file %3").arg(n).arg(__LINE__).arg(__FILE__);
        goto OUT_ERROR;
      }

      if(n < 1)
      {
        if(empty_buf++ > 100)
        {
          break;
        }
      }
      else
      {
        empty_buf = 0;
      }

      for(k=0; k<n; k++)
      {
        if((bytes_rcvd + k) >= mempnts)
        {
          break;
        }

        wavbuf[chn][bytes_rcvd + k] = ((int)(((unsigned char *)device->buf)[k]) - yref[chn] - devparms.yor[chn]) << 5;
      }

      bytes_rcvd += n;

      if(bytes_rcvd >= mempnts)
      {
        break;
      }
    }

    if(bytes_rcvd < mempnts)
    {
      errStr = QString("Download error.  line %1 file %1").arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }
  }

  progress.reset();

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])
    {
      continue;
    }

    usleep(20000);

    tmc_write(QString("WAV:SOUR CHAN%1").arg(chn + 1).toLocal8Bit().constData());

    usleep(20000);

    tmc_write(":WAV:MODE NORM");

    usleep(20000);

    tmc_write(":WAV:STAR 1");

    if(devparms.modelserie == 1)
    {
      usleep(20000);

      tmc_write(":WAV:STOP 1200");
    }
    else
    {
      usleep(20000);

      tmc_write(":WAV:STOP 1400");

      usleep(20000);

      tmc_write(":WAV:POIN 1400");
    }
  }

  if(bytes_rcvd < mempnts)
  {
    errStr = QString("Download error.  line %1 file %2").arg(__LINE__).arg(__FILE__);
    goto OUT_ERROR;
  }
  else
  {
    statusLabel->setText("Downloading finished");
  }

  new UI_wave_window(&devparms, wavbuf, this);

  disconnect(&get_data_thrd, 0, 0, 0);

  scrn_timer->start(devparms.screentimerival);

  return;

OUT_ERROR:

  disconnect(&get_data_thrd, 0, 0, 0);

  progress.reset();

  statusLabel->setText("Downloading aborted");

  if(get_data_thrd.isRunning())
  {
    QMessageBox w_msg_box;
    w_msg_box.setIcon(QMessageBox::NoIcon);
    w_msg_box.setText("Waiting for thread to finish, please wait...");
    w_msg_box.setStandardButtons(QMessageBox::Abort);

    connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));

    w_msg_box.exec();
  }

  if(!progress.wasCanceled())
  {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(errStr);
    msgBox.exec();
  }

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])
    {
      continue;
    }

    tmc_write(QString(":WAV:SOUR CHAN%1").arg(chn + 1).toLocal8Bit().constData());

    tmc_write(":WAV:MODE NORM");

    tmc_write(":WAV:STAR 1");

    if(devparms.modelserie == 1)
    {
      tmc_write(":WAV:STOP 1200");
    }
    else
    {
      tmc_write(":WAV:STOP 1400");

      tmc_write(":WAV:POIN 1400");
    }
  }

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    free(wavbuf[chn]);
    wavbuf[chn] = NULL;
  }

  scrn_timer->start(devparms.screentimerival);
}


void UI_Mainwindow::save_wave_inspector_buffer_to_edf(struct device_settings *d_parms)
{
  int i, j,
      chn,
      chns=0,
      hdl=-1,
      mempnts,
      smps_per_record,
      datrecs=1,
      ret_stat;

  QString errStr, opath;

  long long rec_len=0LL,
            datrecduration;

  QMessageBox wi_msg_box;

  save_data_thread sav_data_thrd(1);

  mempnts = d_parms->acquirememdepth;

  smps_per_record = mempnts;

  for(i=0; i<MAX_CHNS; i++)
  {
    if(!d_parms->chandisplay[i])
    {
      continue;
    }

    chns++;
  }

  if(!chns)
  {
    errStr = "No active channels.";
    goto OUT_ERROR;
  }

  while(smps_per_record >= (5000000 / chns))
  {
    smps_per_record /= 2;

    datrecs *= 2;
  }

  rec_len = (EDFLIB_TIME_DIMENSION * (long long)mempnts) / d_parms->samplerate;

  if(rec_len < 100)
  {
    errStr = "Can not save waveforms shorter than 10 uSec.\n"
             "Select a higher memory depth or a higher timebase.";

    goto OUT_ERROR;
  }

  if(!recent_savedir.isEmpty())
  {
    opath = recent_savedir + "/";
  }
  opath += "waveform.edf";

  opath = QFileDialog::getSaveFileName(this, "Save file", opath, "EDF files (*.edf *.EDF)");

  if(opath.isEmpty())
  {
    goto OUT_NORMAL;
  }

  recent_savedir = directory_from_path(opath);

  hdl = edfopen_file_writeonly(opath.toLocal8Bit().constData(), EDFLIB_FILETYPE_EDFPLUS, chns);
  if(hdl < 0)
  {
    errStr = "Can not create EDF file.";
    goto OUT_ERROR;
  }

  statusLabel->setText("Saving EDF file...");

  datrecduration = (rec_len / 10LL) / datrecs;

//  printf("rec_len: %lli   datrecs: %i   datrecduration: %lli\n", rec_len, datrecs, datrecduration);

  if(datrecduration < 10000LL)
  {
    if(edf_set_micro_datarecord_duration(hdl, datrecduration))
    {
      errStr = QString("Can not set datarecord duration of EDF file: %1").arg(datrecduration);
      qDebug() << QString("line %1: rec_len: %2   datrecs: %3   datrecduration: %4")
        .arg(__LINE__).arg(rec_len).arg(datrecs).arg(datrecduration);
      goto OUT_ERROR;
    }
  }
  else
  {
    if(edf_set_datarecord_duration(hdl, datrecduration / 10LL))
    {
      errStr = QString("Can not set datarecord duration of EDF file: %1").arg(datrecduration);
      qDebug() << QString("line %1: rec_len: %2   datrecs: %3   datrecduration: %4")
          .arg(__LINE__).arg(rec_len).arg(datrecs).arg(datrecduration);
      goto OUT_ERROR;
    }
  }

  j = 0;

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!d_parms->chandisplay[chn])
    {
      continue;
    }

    edf_set_samplefrequency(hdl, j, smps_per_record);
    edf_set_digital_maximum(hdl, j, 32767);
    edf_set_digital_minimum(hdl, j, -32768);
    if(d_parms->chanscale[chn] > 2)
    {
      edf_set_physical_maximum(hdl, j, d_parms->yinc[chn] * 32767.0 / 32.0);
      edf_set_physical_minimum(hdl, j, d_parms->yinc[chn] * -32768.0 / 32.0);
      edf_set_physical_dimension(hdl, j, "V");
    }
    else
    {
      edf_set_physical_maximum(hdl, j, 1000.0 * d_parms->yinc[chn] * 32767.0 / 32.0);
      edf_set_physical_minimum(hdl, j, 1000.0 * d_parms->yinc[chn] * -32768.0 / 32.0);
      edf_set_physical_dimension(hdl, j, "mV");
    }
    edf_set_label(hdl, j, QString("CHAN%1").arg(chn + 1).toLocal8Bit().constData());

    j++;
  }

  edf_set_equipment(hdl, d_parms->modelname.toLocal8Bit().constData());

//  printf("datrecs: %i    smps_per_record: %i\n", datrecs, smps_per_record);

  sav_data_thrd.init_save_memory_edf_file(d_parms, hdl, datrecs, smps_per_record, d_parms->wavebuf);

  wi_msg_box.setIcon(QMessageBox::NoIcon);
  wi_msg_box.setText("Saving EDF file ...");
  wi_msg_box.setStandardButtons(QMessageBox::Abort);

  connect(&sav_data_thrd, SIGNAL(finished()), &wi_msg_box, SLOT(accept()));

  sav_data_thrd.start();

  ret_stat = wi_msg_box.exec();

  if(ret_stat != QDialog::Accepted)
  {
    errStr = "Saving EDF file aborted.";
    goto OUT_ERROR;
  }

  if(sav_data_thrd.get_error_num())
  {
    errStr = sav_data_thrd.get_error_str();
    goto OUT_ERROR;
  }

OUT_NORMAL:

  disconnect(&sav_data_thrd, 0, 0, 0);

  if(hdl >= 0)
  {
    edfclose_file(hdl);
  }

  if(!opath.isEmpty())
  {
    statusLabel->setText("Save file canceled.");
  }
  else
  {
    statusLabel->setText("Saved memory buffer to EDF file.");
  }

  return;

OUT_ERROR:

  disconnect(&sav_data_thrd, 0, 0, 0);

  statusLabel->setText("Saving file aborted.");

  if(hdl >= 0)
  {
    edfclose_file(hdl);
  }

  wi_msg_box.setIcon(QMessageBox::Critical);
  wi_msg_box.setText(errStr);
  wi_msg_box.setStandardButtons(QMessageBox::Ok);
  wi_msg_box.exec();
}


//     tmc_write(":WAV:PRE?");
//
//     n = tmc_read();
//
//     if(n < 0)
//     {
//       strlcpy(str, "Can not read from device.");
//       goto OUT_ERROR;
//     }

//     printf("waveform preamble: %s\n", device->buf);

//     if(parse_preamble(device->buf, device->sz, &wfp, i))
//     {
//       strlcpy(str, "Preamble parsing error.");
//       goto OUT_ERROR;
//     }

//     printf("waveform preamble:\n"
//            "format: %i\n"
//            "type: %i\n"
//            "points: %i\n"
//            "count: %i\n"
//            "xincrement: %e\n"
//            "xorigin: %e\n"
//            "xreference: %e\n"
//            "yincrement: %e\n"
//            "yorigin: %e\n"
//            "yreference: %e\n",
//            wfp.format, wfp.type, wfp.points, wfp.count,
//            wfp.xincrement[i], wfp.xorigin[i], wfp.xreference[i],
//            wfp.yincrement[i], wfp.yorigin[i], wfp.yreference[i]);

//     rec_len = wfp.xincrement[i] * wfp.points;





void UI_Mainwindow::save_screen_waveform()
{
  int i, j,
      n=0,
      chn,
      chns=0,
      hdl=-1,
      yref[MAX_CHNS];


  QString errStr, opath;

  short *wavbuf[MAX_CHNS];

  long long rec_len=0LL;

  if(device == NULL)
  {
    return;
  }

  for(i=0; i<MAX_CHNS; i++)
  {
    wavbuf[i] = NULL;
  }

  save_data_thread get_data_thrd(0);

  QMessageBox w_msg_box;
  w_msg_box.setIcon(QMessageBox::NoIcon);
  w_msg_box.setText("Downloading data...");
  w_msg_box.setStandardButtons(QMessageBox::Abort);

  scrn_timer->stop();

  scrn_thread->wait();

  if(devparms.timebasedelayenable)
  {
    rec_len = EDFLIB_TIME_DIMENSION * devparms.timebasedelayscale * devparms.hordivisions;
  }
  else
  {
    rec_len = EDFLIB_TIME_DIMENSION * devparms.timebasescale * devparms.hordivisions;
  }

  if(rec_len < 10LL)
  {
    errStr =  "Can not save waveforms when timebase < 1uSec.";
    goto OUT_ERROR;
  }

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])  // Download data only when channel is switched on
    {
      continue;
    }

    wavbuf[chn] = (short *)malloc(WAVFRM_MAX_BUFSZ * sizeof(short));
    if(wavbuf[chn] == NULL)
    {
      errStr = "Malloc error.";
      goto OUT_ERROR;
    }

    chns++;
  }

  if(!chns)
  {
    errStr = "No active channels.";
    goto OUT_ERROR;
  }

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])  // Download data only when channel is switched on
    {
      continue;
    }

    usleep(20000);

    tmc_write(QString(":WAV:SOUR CHAN%i").arg(chn + 1).toLocal8Bit().constData());

    usleep(20000);

    tmc_write(":WAV:FORM BYTE");

    usleep(20000);

    tmc_write(":WAV:MODE NORM");

    usleep(20000);

    tmc_write(":WAV:YINC?");

    usleep(20000);

    tmc_read();

    devparms.yinc[chn] = atof(device->buf);

    if(devparms.yinc[chn] < 1e-6)
    {
      errStr = QString("Error, parameter \"YINC\" out of range: %1  line %2 file %3").arg(devparms.yinc[chn])
          .arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }

    usleep(20000);

    tmc_write(":WAV:YREF?");

    usleep(20000);

    tmc_read();

    yref[chn] = atoi(device->buf);

    if((yref[chn] < 1) || (yref[chn] > 255))
    {
      errStr = QString("Error, parameter \"YREF\" out of range: %1  line %2 file %3").arg(yref[chn])
          .arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }

    usleep(20000);

    tmc_write(":WAV:YOR?");

    usleep(20000);

    tmc_read();

    devparms.yor[chn] = atoi(device->buf);

    if((devparms.yor[chn] < -255) || (devparms.yor[chn] > 255))
    {
      errStr = QString("Error, parameter \"YOR\" out of range: %1  line %2 file %3").arg(devparms.yor[chn])
          .arg(__LINE__).arg(__FILE__);
      goto OUT_ERROR;
    }

    usleep(20000);

    tmc_write(":WAV:DATA?");

    connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));

    get_data_thrd.start();

    if(w_msg_box.exec() != QDialog::Accepted)
    {
      disconnect(&get_data_thrd, 0, 0, 0);
      errStr = "Aborted by user.";
      goto OUT_ERROR;
    }

    disconnect(&get_data_thrd, 0, 0, 0);

    n = get_data_thrd.get_num_bytes_rcvd();
    if(n < 0)
    {
      errStr =  "Can not read from device.";
      goto OUT_ERROR;
    }

    if(n > WAVFRM_MAX_BUFSZ)
    {
      errStr = QString("Datablock too big for buffer: %1").arg(n);
      goto OUT_ERROR;
    }

    if(n < 16)
    {
      errStr = "Not enough data in buffer.";
      goto OUT_ERROR;
    }

    for(i=0; i<n; i++)
    {
      wavbuf[chn][i] = ((int)(((unsigned char *)device->buf)[i]) - yref[chn] - devparms.yor[chn]) << 5;
    }
  }

  if(!recent_savedir.isEmpty())
  {
    opath = recent_savedir += "/";
  }
  opath +=  "waveform.edf";

  opath = QFileDialog::getSaveFileName(this, "Save file", opath, "EDF files (*.edf *.EDF)");

  // TODO: these exit looks strange, verify if this is correct
  if(opath.isEmpty())
  {
    goto OUT_NORMAL;
  }

  recent_savedir = directory_from_path(recent_savedir);

  hdl = edfopen_file_writeonly(opath.toLocal8Bit().constData(), EDFLIB_FILETYPE_EDFPLUS, chns);
  if(hdl < 0)
  {
    errStr = "Can not create EDF file.";
    goto OUT_ERROR;
  }

  if(edf_set_datarecord_duration(hdl, rec_len / 100LL))
  {
    errStr = QString("Can not set datarecord duration of EDF file: %1").arg(rec_len / 100LL);
    goto OUT_ERROR;
  }

  j = 0;

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])
    {
      continue;
    }

    edf_set_samplefrequency(hdl, j, n);
    edf_set_digital_maximum(hdl, j, 32767);
    edf_set_digital_minimum(hdl, j, -32768);
    if(devparms.chanscale[chn] > 2)
    {
      edf_set_physical_maximum(hdl, j, devparms.yinc[chn] * 32767.0 / 32.0);
      edf_set_physical_minimum(hdl, j, devparms.yinc[chn] * -32768.0 / 32.0);
      edf_set_physical_dimension(hdl, j, "V");
    }
    else
    {
      edf_set_physical_maximum(hdl, j, 1000.0 * devparms.yinc[chn] * 32767.0 / 32.0);
      edf_set_physical_minimum(hdl, j, 1000.0 * devparms.yinc[chn] * -32768.0 / 32.0);
      edf_set_physical_dimension(hdl, j, "mV");
    }
    edf_set_label(hdl, j, QString("CHAN%1").arg(chn + 1).toLocal8Bit().constData());

    j++;
  }

  edf_set_equipment(hdl, devparms.modelname.toLocal8Bit().constData());

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    if(!devparms.chandisplay[chn])
    {
      continue;
    }

    if(edfwrite_digital_short_samples(hdl, wavbuf[chn]))
    {
      errStr = "A write error occurred.";
      goto OUT_ERROR;
    }
  }

OUT_NORMAL:

  if(hdl >= 0)
  {
    edfclose_file(hdl);
  }

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    free(wavbuf[chn]);
    wavbuf[chn] = NULL;
  }

  scrn_timer->start(devparms.screentimerival);

  return;

OUT_ERROR:

  if(!get_data_thrd.isFinished())
  {
    connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));
    w_msg_box.setText("Waiting for thread to finish, please wait...");
    w_msg_box.exec();
  }

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Critical);
  msgBox.setText(errStr);
  msgBox.exec();

  if(hdl >= 0)
  {
    edfclose_file(hdl);
  }

  for(chn=0; chn<MAX_CHNS; chn++)
  {
    free(wavbuf[chn]);
    wavbuf[chn] = NULL;
  }

  scrn_timer->start(devparms.screentimerival);
}

















